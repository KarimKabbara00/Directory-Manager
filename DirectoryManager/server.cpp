#include "Server.h"
#include <string>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <thread>

#include <QtSql>
#include <QSqlDatabase>

int Server::activeUserCount;
int Server::activeReposCount;
std::vector<Repository> Server::repositoryList;
std::vector<User> Server::userList;
std::map<int, std::vector<QString>> Server::connectedClientsMap;
std::map<int, std::map<User*, Repository*>> Server::activeRepos;
QSqlDatabase Server::database;
std::string Server::parentPath;
std::vector<QString> Server::activeAdmin;

Server::Server() {
    int wsa = WSAStartup(MAKEWORD(2, 2), &this->wsaData);
    this->portNumber = 5555;
    this->server = socket(AF_INET, SOCK_STREAM, 0);
    this->serverAddress.sin_addr.s_addr = INADDR_ANY;
    this->serverAddress.sin_family = AF_INET;
    this->serverAddress.sin_port = htons(this->portNumber);
    this->keepServerRunning = true;
    Server::parentPath = "C:\\Users\\karim\\Desktop\\Repositories\\";
    ::bind(this->server, reinterpret_cast<SOCKADDR*>(&this->serverAddress), sizeof(this->serverAddress));  // bind socket
    Server::loadData();

    Server::database = QSqlDatabase::addDatabase("QSQLITE", "MainThread");
    Server::database.setDatabaseName("C:/Users/karim/Desktop/DMDB.sqlite");
    if (Server::database.open()){
        qDebug() << "opened";
    }

    else
        qDebug() << "not opened";

}

void Server::run() {
    while (this->keepServerRunning) {

        SOCKET client;
        SOCKADDR_IN clientAddress;
        int clientAddressSize;

        // wait for connection
        if (listen(this->server, 0) == SOCKET_ERROR) {
            break;
        }

        clientAddressSize = sizeof(clientAddress);
        if ((client = accept(this->server, reinterpret_cast<SOCKADDR*>(&this->clientAddress), &clientAddressSize)) != INVALID_SOCKET) {
            qDebug () << "Client Connected!";
            int id = ++Server::activeUserCount;
            ClientWorker *c = new ClientWorker(client, id);
            std::thread newClient(&ClientWorker::run, *c);
            newClient.detach();
        }
    }
}

bool Server::adminSignIn(std::string username, std::string password){

    QSqlQuery adminSignInQuery(Server::database);
    QString query = "SELECT * FROM Users WHERE Email = '" + QString::fromStdString(username) + "' AND Password = '" + QString::fromStdString(password) + "';";
    adminSignInQuery.prepare(query);

    if(!adminSignInQuery.exec()){
        qDebug() << "failed to exec";
    }
    else{
        while(adminSignInQuery.next()){
            if (adminSignInQuery.value(3).toString() == QString::fromStdString(username) &&
                adminSignInQuery.value(4).toString() == QString::fromStdString(password) &&
                adminSignInQuery.value(5) == true){
                Server::activeAdmin.emplace_back(adminSignInQuery.value(0).toString());
                Server::activeAdmin.emplace_back(adminSignInQuery.value(1).toString());
                Server::activeAdmin.emplace_back(adminSignInQuery.value(2).toString());
                Server::activeAdmin.emplace_back(adminSignInQuery.value(3).toString());
                Server::activeAdmin.emplace_back(adminSignInQuery.value(4).toString());
                Server::activeAdmin.emplace_back(adminSignInQuery.value(5).toString());
                return true;
            }
        }
    }
    return false;
}

void Server::addAdmin(std::string fName, std::string lName, std::string email, std::string password){

    int ID = 0;
    QSqlQuery nextID;
    nextID.prepare("SELECT Count(*) FROM Users");
    nextID.exec();
    if(nextID.next())
        ID = nextID.value(0).toInt();

    QSqlQuery addAdminQuery(Server::database);
    addAdminQuery.prepare("INSERT INTO Users ("
                  "ID,"
                  "firstName,"
                  "lastName,"
                  "Email,"
                  "Password,"
                  "isAdmin)"
                  "VALUES (?,?,?,?,?,?);");

    addAdminQuery.addBindValue(ID);
    addAdminQuery.addBindValue(QString::fromStdString(fName));
    addAdminQuery.addBindValue(QString::fromStdString(lName));
    addAdminQuery.addBindValue(QString::fromStdString(email));
    addAdminQuery.addBindValue(QString::fromStdString(password));
    addAdminQuery.addBindValue(1);

    if(!addAdminQuery.exec()){
        qDebug() << "failed to exec";
    }

    int id = this->userList.size();
    this->userList.emplace_back(User(id, fName, lName, email, password, true));
}

User Server::getUserByID(int id){
    for (auto& u : Server::userList) {
        if(u.getID() == id){
            return u;
        }
    }
}

bool Server::createRepository(std::string name, bool isPrivate){

    std::string path = Server::parentPath + "\\" + name;
    int isCreated = _wmkdir(std::wstring(path.begin(), path.end()).c_str());
    if (!isCreated) {
        int ID = 0;
        QSqlQuery nextID(Server::database);
        nextID.prepare("SELECT Count(*) FROM Repositories");
        nextID.exec();
        if(nextID.next())
            ID = nextID.value(0).toInt();

        QSqlQuery addRepo(Server::database);
        addRepo.prepare("INSERT INTO Repositories ("
                        "ID,"
                        "OwnerID,"
                        "Name,"
                        "Directory,"
                        "isPrivate)"
                        "VALUES (?,?,?,?,?);");

          addRepo.addBindValue(ID);
          addRepo.addBindValue(Server::activeAdmin[0]);
          addRepo.addBindValue(QString::fromStdString(name));
          addRepo.addBindValue(QString::fromStdString(path));
          addRepo.addBindValue(isPrivate);

          if (!addRepo.exec())
              return false;
          else
              return true;
    }    
    return false;
}

bool Server::createFile(std::string repoPath, std::string fileName, std::string repoName){
    for (auto& path : std::filesystem::recursive_directory_iterator(repoPath))
        if (path.is_regular_file()){
            if (path.path().string() == repoPath + fileName){
                return false;
            }
        }

    std::ofstream newFile (repoPath + fileName);
    newFile.close();

    int ID;
    QSqlQuery repoID(Server::database);
    repoID.prepare("SELECT ID FROM Repositories WHERE Name = '" + QString::fromStdString(repoName) + "';");
    repoID.exec();
    if(repoID.next())
        ID = repoID.value(0).toInt();
    else
        return false;


    // Save file to Database
    QSqlQuery addFile(Server::database);
    QFile file(QString::fromStdString(repoPath + fileName));
    QByteArray fileData = file.readAll();

    addFile.prepare("INSERT INTO RepoFiles ("
                    "ID,"
                    "File)"
                    "VALUES (?,?);");

    addFile.addBindValue(ID);
    addFile.addBindValue(fileData);

    if (!addFile.exec()){
        return false;
    }
    else{
        return true;
    }
}

bool Server::createFolder(std::string repoPath, std::string fileName){
    std::string path = repoPath + fileName;
    int isCreated = _wmkdir(std::wstring(path.begin(), path.end()).c_str());
    if(!isCreated)
        return true;
    else
        return false;
}

bool Server::createRootFolder(std::string folderPath){
    std::string path = Server::parentPath + folderPath;
    int isCreated = _wmkdir(std::wstring(path.begin(), path.end()).c_str());
    if(!isCreated)
        return true;
    else
        return false;
}

std::string Server::constructMessage(std::initializer_list<std::string> il){
    std::string message;
    for (auto const& x: il)
        message += (x + "|");
    return message;
}

std::vector<std::string> Server::deconstructMessage(std::string message){

    std::vector<std::string> m;
    std::string delim = "|";
    auto start = 0U;
    auto end = message.find(delim);
    while (end != std::string::npos){
        m.emplace_back(message.substr(start, end - start));
        start = end + delim.length();
        end = message.find(delim, start);
    }
    m.emplace_back(message.substr(start, end - start));
    return m;
}

bool Server::addContributor(QString repoName, QString email, QString permission){

    int repositoryID;
    int userID;

    QSqlQuery repoID(Server::database);
    repoID.prepare("SELECT ID FROM Repositories WHERE Name = '" + repoName + "';");
    if (repoID.exec()){
        while (repoID.next()){
            repositoryID = repoID.value(0).toInt();
        }
    }

    QSqlQuery uID(Server::database);
    uID.prepare("SELECT ID FROM Users WHERE Email = '" + email + "';");
    if (uID.exec()){
        while (uID.next()){
            userID = uID.value(0).toInt();
        }
    }

    int ID = 0;
    QSqlQuery nextID(Server::database);
    nextID.prepare("SELECT Count(*) FROM RepoContributors");
    nextID.exec();
    if(nextID.next())
        ID = nextID.value(0).toInt();

    // Create new contributor
    QSqlQuery addContrib(Server::database);
    addContrib.prepare("INSERT INTO RepoContributors ("
                  "ID,"
                  "UserID,"
                  "RepositoryID,"
                  "Permission)"
                  "VALUES (?,?,?,?);");

    addContrib.addBindValue(ID);
    addContrib.addBindValue(userID);
    addContrib.addBindValue(repositoryID);
    addContrib.addBindValue(permission);

    if(!addContrib.exec()){
        return false;
    }
    else{
        return true;
    }
}

bool Server::removeContributor(QString repoName, QString email, QString reason){

    QString uID;

    // get user id from email
    QSqlQuery findUser(Server::database);
    findUser.prepare("Select ID from Users WHERE Email = '" + email + "';");
    if(findUser.exec())
        while(findUser.next())
            uID = findUser.value(0).toString();


    // remove user from repoContrib
    QSqlQuery remContrib(Server::database);
    remContrib.prepare("DELETE FROM RepoContributors WHERE UserID = " + uID + ";");

    if(!remContrib.exec())
        return false;
    else
        return true;

}

void Server::shutdown() {
    this->keepServerRunning = false;
    closesocket(this->server);
    WSACleanup();
}
///////////////////////////////////////////////////////

QDataStream& operator<<(QDataStream& out, const std::vector<std::string>& info){
    out << QString::fromStdString(info[0]) << QString::fromStdString(info[1]) << QString::fromStdString(info[2])
        << QString::fromStdString(info[3]) << QString::fromStdString(info[4]) << QString::fromStdString(info[5])
        << QString::fromStdString(info[6]) << QString::fromStdString(info[7]) << QString::fromStdString(info[8])
        << QString::fromStdString("\n");

    return out;
}

QDataStream& operator>>(QDataStream& in, std::vector<std::string>& info){

    QString arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10;
    /*  >> is overloaded for serialization
        of users and repositories.

        arg         user        repo
        ----------------------------
        arg1    |   uID     |   uID
        arg2    |   fName   |   name
        arg3    |   lName   |   isPriv
        arg4    |   email   |   contribs
        arg5    |   pass    |   perms
        arg6    |   Admin   |   directory
        arg7    |   repos   |   day
        arg8    |   perms   |   month
        arg9    |   N/A     |   year
        arg10   |   \n      |   \n        (not emplaced in vector)
    */
    in >> arg1 >> arg2 >> arg3 >> arg4 >> arg5 >> arg6 >> arg7 >> arg8 >> arg9 >> arg10;

    info.emplace_back(arg1.toStdString());
    info.emplace_back(arg2.toStdString());
    info.emplace_back(arg3.toStdString());
    info.emplace_back(arg4.toStdString());
    info.emplace_back(arg5.toStdString());
    info.emplace_back(arg6.toStdString());
    info.emplace_back(arg7.toStdString());
    info.emplace_back(arg8.toStdString());
    info.emplace_back(arg9.toStdString());

    return in;
}

void Server::serializeRepositories() {

    QMap<int, std::vector<std::string>> emp;  // emp holds id of repo, and info
    for (int i = 0; i < Server::repositoryList.size(); ++i) {
        std::vector<std::string> info;
        std::string contributors;
        std::string permissions;

        info.emplace_back(std::to_string(Server::repositoryList[i].getOwner()));
        info.emplace_back(Server::repositoryList[i].getName());

        if (Server::repositoryList[i].getIsPrivate())
            info.emplace_back("true");
        else
            info.emplace_back("false");

        for (int j = 0; j < Server::repositoryList[i].getContributors().size(); j++){
            contributors += (std::to_string(Server::repositoryList[i].getContributors()[j]) + ",");
            permissions += (std::to_string(Server::repositoryList[i].getPermissions()[j]) + ",");
        }
        info.emplace_back(contributors);
        info.emplace_back(permissions);

        info.emplace_back(Server::repositoryList[i].getDirectory());
        info.emplace_back(std::to_string(Server::repositoryList[i].getDayCreated()));
        info.emplace_back(std::to_string(Server::repositoryList[i].getMonthCreated()));
        info.emplace_back(std::to_string(Server::repositoryList[i].getYearCreated()));

        emp.insert(i, info); // for each repo, insert repo and its ID
    }

    QFile file("C:\\Users\\karim\\Desktop\\Repositories\\repos.txt");    // file that holds repos
    if (!file.open(QIODevice::WriteOnly)) {  // try to open
        qDebug() << "Error! Cannot open file!";
    }
//    file.resize(0); // clear file before writing

    QDataStream outStream(&file);   // outstream to write to file
    outStream.setVersion(QDataStream::Qt_6_4);
    outStream << emp;   // write to file

    file.flush();   // save
    file.close();   // close
}

void Server::deserializeRepositories() {
    QMap<int, std::vector<std::string>> emp;

    QFile file("C:\\Users\\karim\\Desktop\\Repositories\\repos.txt");
    if (!file.open(QIODevice::ReadOnly)) {  // try to open
        qDebug() << "Error! Cannot open file!";
    }

    QDataStream inStream(&file);   // outstream to write to file
    inStream.setVersion(QDataStream::Qt_6_4);
    inStream >> emp;   // write to file

    for (int i = 0; i < emp.size(); i++){

        // Get user
        User user;
        for (int j = 0; j < Server::userList.size(); j++){
            if (Server::userList[j].getID() == std::stoi(emp[i][0])) {
                user = Server::userList[j];
            }
        }

        // get repo name
        std::string repoName = emp[i][1];

        // get is private
        boolean isPrivate;
        if (emp[i][2] == "false")
            isPrivate = false;
        else
            isPrivate = true;

        std::vector<int> contribs;
        std::vector<int> perms;

        for (int j = 0; j < emp[i][3].size(); ++j) {
            if (emp[i][3][j] != ',') {
                contribs.emplace_back(emp[i][3][j] - '0');
                perms.emplace_back(emp[i][4][j] - '0');
            }
        }

        std::string dir = emp[i][5];        // directory path
        int day = std::stoi(emp[i][6]);     // d/m/y created
        int month = std::stoi(emp[i][7]);
        int year = std::stoi(emp[i][8]);

        // create and store repo
        Server::repositoryList.clear();
        Server::repositoryList.emplace_back(Repository(user, repoName, isPrivate, dir, contribs, perms, day, month, year));
    }
}

void Server::serializeUsers() {

    QMap<int, std::vector<std::string>> emp;  // emp holds id of repo, and reop
    for (int i = 0; i < Server::userList.size(); ++i) {
        std::vector<std::string> info;
        std::string repos;
        std::string permissions;

        info.emplace_back(std::to_string(Server::userList[i].getID()));
        info.emplace_back(Server::userList[i].getFirstName());
        info.emplace_back(Server::userList[i].getLastName());
        info.emplace_back(Server::userList[i].getEmail());
        info.emplace_back(Server::userList[i].getPassword());

        if (Server::userList[i].getIsAdmin())
            info.emplace_back("true");
        else
            info.emplace_back("false");

        for (int j = 0; j < Server::userList[i].getRepoList().size(); j++) {
            repos += (Server::userList[i].getRepoList()[j].getName() + ",");
            permissions += (std::to_string(Server::userList[i].getPermissionsList()[j]) + ",");
        }
        info.emplace_back(repos);
        info.emplace_back(permissions);


        // overloaded << needs 9 vector slots. User only has 8
        info.emplace_back("");

        emp.insert(i, info); // for each repo, insert repo and its ID
    }

    QFile file("C:\\Users\\karim\\Desktop\\Repositories\\users.txt");    // file that holds repos
    if (!file.open(QIODevice::WriteOnly)) {  // try to open
        qDebug() << "Error! Cannot open file!";
    }
//    file.resize(0); // clear file before writing

    QDataStream outStream(&file);   // outstream to write to file
    outStream.setVersion(QDataStream::Qt_6_4);
    outStream << emp;   // write to file

    file.flush();   // save
    file.close();   // close
}

void Server::deserializeUsers() {

    QMap<int, std::vector<std::string>> emp;

    QFile file("C:\\Users\\karim\\Desktop\\Repositories\\users.txt");
    if (!file.open(QIODevice::ReadOnly)) {  // try to open
        qDebug() << "Error! Cannot open file!";
    }

    QDataStream inStream(&file);   // outstream to write to file
    inStream.setVersion(QDataStream::Qt_6_4);
    inStream >> emp;   // write to file

    for (int i = 0; i < emp.size(); i++) {

        // Get info
        int id = std::stoi(emp[i][0]);
        std::string fName = emp[i][1];
        std::string lName = emp[i][2];
        std::string userEmail = emp[i][3];
        std::string pass = emp[i][4];
        bool isAdmin;

        if (emp[i][5] == "true")
            isAdmin = true;
        else
            isAdmin = false;

        // Users must be deserialized before repos are.
        // This means, each user's repos are empty upon
        // derserialization. They will be loaded right
        // after repos are deserialized.
        std::vector<Repository> repos;
        std::vector<int> perms;

        Server::userList.clear();
        Server::userList.emplace_back(User(id, fName, lName, userEmail, pass, isAdmin, repos, perms));
    }
}

void Server::saveData() {
    this->serializeRepositories();
    this->serializeUsers();
}

void Server::loadData() {
    this->deserializeUsers();
    this->deserializeRepositories();
    for (int i = 0; i < Server::userList.size(); i++){
        for (int j = 0; j < Server::repositoryList.size(); j++) {
            for (int k = 0; k < Server::repositoryList[j].getContributors().size(); k++){
                if (Server::userList[i].getID() == Server::repositoryList[j].getContributors()[k]) {
                    Server::userList[i].addRepo(Server::repositoryList[j]);
                    Server::userList[i].addPerm(Server::repositoryList[j].getPermissions()[k]);
                }
            }
        }
    }
}
