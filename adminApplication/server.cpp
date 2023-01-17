#include "Server.h"

int Server::activeUserCount;
std::map<int, std::vector<QString>> Server::connectedClientsMap;
QSqlDatabase Server::database;
std::string Server::parentPath;
std::vector<QString> Server::activeAdmin;

#define HOST "" // HOST NAME HERE
#define PORT    // PORT NUMBER HERE
#define USER "" // DB USERNAME HERE
#define PASS "" // DB PASSWORD HERE
#define NAME "" // DB NAME HERE

Server::Server() {
    int wsa = WSAStartup(MAKEWORD(2, 2), &this->wsaData);
    if (wsa)
        return;
    this->portNumber = 5555;
    this->server = socket(AF_INET, SOCK_STREAM, 0);
    this->serverAddress.sin_addr.s_addr = INADDR_ANY;
    this->serverAddress.sin_family = AF_INET;
    this->serverAddress.sin_port = htons(this->portNumber);
    this->keepServerRunning = true;
    Server::parentPath = ""; // PATH TO SERVER DIRECTORY
    ::bind(this->server, reinterpret_cast<SOCKADDR*>(&this->serverAddress), sizeof(this->serverAddress));  // bind socket

    // connect to Database
    Server::database = QSqlDatabase::addDatabase("QPSQL", "MainThread");
    Server::database.setHostName(HOST);
    Server::database.setPort(PORT);
    Server::database.setUserName(USER);
    Server::database.setPassword(PASS);
    Server::database.setDatabaseName(NAME);
    if (Server::database.open())
        qDebug() << "Opened Database";
    else
        qDebug() << "Couldnt Open Database";

    // thread to update tables
    std::thread fileBackup(&Server::backupFiles, this);
    fileBackup.detach();
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

    QByteArray passByteArr(password.c_str(), password.length());
    QString hashedPass = QString(QCryptographicHash::hash((passByteArr),QCryptographicHash::Md5).toHex());

    QSqlQuery adminSignInQuery(Server::database);
    QString query = "SELECT * FROM public.\"Users\" WHERE \"Email\" = '" + QString::fromStdString(username) + "' AND \"Password\" = '" + hashedPass + "'; ";

    adminSignInQuery.prepare(query);
    if(!adminSignInQuery.exec()){
        qDebug() << "failed to exec";
        qDebug() << adminSignInQuery.lastError();
    }
    else{
        while(adminSignInQuery.next()){
            if (adminSignInQuery.value(3).toString() == QString::fromStdString(username) &&
                adminSignInQuery.value(4).toString() == hashedPass &&
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

bool Server::addAdmin(std::string fName, std::string lName, std::string email, std::string password){



    // check email format
    if(!Server::isEmailValid(email))
        return false;

    if (!Server::checkPassword(password))
        return false;

    QByteArray passByteArr(password.c_str(), password.length());
    QString hashedPass = QString(QCryptographicHash::hash((passByteArr),QCryptographicHash::Md5).toHex());

    int ID = 0;
    QSqlQuery nextID(Server::database);
    nextID.prepare("SELECT Count(*) FROM public.\"Users\"");
    nextID.exec();
    if(nextID.next())
        ID = nextID.value(0).toInt();

    QSqlQuery addAdminQuery(Server::database);
    addAdminQuery.prepare("INSERT INTO public.\"Users\" ("
                          "\"ID\","
                          "\"firstName\","
                          "\"lastName\","
                          "\"Email\","
                          "\"Password\","
                          "\"isAdmin\")"
                          "VALUES (?,?,?,?,?,?);");

    addAdminQuery.addBindValue(ID);
    addAdminQuery.addBindValue(QString::fromStdString(fName));
    addAdminQuery.addBindValue(QString::fromStdString(lName));
    addAdminQuery.addBindValue(QString::fromStdString(email));
    addAdminQuery.addBindValue(hashedPass);
    addAdminQuery.addBindValue(1);

    if(!addAdminQuery.exec()){
        qDebug() << "failed to exec";
        return false;
    }

    return true;
}

bool Server::createRepository(std::string name, bool isPrivate){

    std::string path = Server::parentPath + "\\" + name;
    int isCreated = _wmkdir(std::wstring(path.begin(), path.end()).c_str());
    if (!isCreated) {
        int ID = 0;
        QSqlQuery nextID(Server::database);
        nextID.prepare("SELECT Count(*) FROM public.\"Repositories\" ");
        nextID.exec();
        if(nextID.next())
            ID = nextID.value(0).toInt();

        QSqlQuery addRepo(Server::database);
        addRepo.prepare("INSERT INTO public.\"Repositories\" ("
                        "\"ID\","
                        "\"OwnerID\","
                        "\"Name\","
                        "\"Directory\","
                        "\"isPrivate\")"
                        "VALUES (?,?,?,?,?);");

          addRepo.addBindValue(ID);
          addRepo.addBindValue(Server::activeAdmin[0]);
          addRepo.addBindValue(QString::fromStdString(name));
          addRepo.addBindValue(QString::fromStdString(path));
          addRepo.addBindValue(isPrivate);

          if (!addRepo.exec()){
              qDebug() << addRepo.lastError();
              return false;
          }
          else
              return true;
    }    
    return false;
}

bool Server::createFile(std::string repoPath, std::string fileName, std::string repoName){
    repoPath.replace(repoPath.begin(), repoPath.begin() + 36, Server::parentPath);
    for (auto& path : std::filesystem::recursive_directory_iterator(repoPath)){
        if (path.is_regular_file()){
            if (path.path().string() == repoPath + fileName){
                return false;
            }
        }
    }

    std::ofstream newFile (repoPath + fileName);
    newFile.close();

    // primary key RepoFiles
    int ID = 0;
    QSqlQuery nextID(Server::database);
    nextID.prepare("SELECT MAX(\"ID\") FROM public.\"RepoFiles\"");
    nextID.exec();
    if(nextID.next())
        ID = nextID.value(0).toInt() + 1;
    else{
        qDebug() << nextID.lastError();
        return false;
    }

    // RepoID for RepoFiles
    int repositoryID;
    QSqlQuery repoID(Server::database);
    repoID.prepare("SELECT \"ID\" FROM public.\"Repositories\" WHERE \"Name\" = '" + QString::fromStdString(repoName) + "';");
    repoID.exec();
    if(repoID.next())
        repositoryID = repoID.value(0).toInt();
    else{
        qDebug() << repoID.lastError();
        return false;
    }


    // Save file to Database
    std::string filePath = repoPath + fileName;
    QSqlQuery addFile(Server::database);
    QFile file(QString::fromStdString(filePath));
    file.open(QIODevice::ReadOnly);
    QByteArray fileData = file.readAll();

    addFile.prepare("INSERT INTO public.\"RepoFiles\" ("
                    "\"ID\","
                    "\"RepoID\","
                    "\"FileName\","
                    "\"File\")"
                    "VALUES (?,?,?,?);");


    std::replace(filePath.begin(), filePath.end(), '\\', '/');
    addFile.addBindValue(ID);
    addFile.addBindValue(repositoryID);
    addFile.addBindValue(QString::fromStdString(filePath));
    addFile.addBindValue(fileData);

    if (!addFile.exec()){
        qDebug() << addFile.lastError();
        return false;
    }
    else{
        return true;
    }
}

bool Server::createFolder(std::string repoPath, std::string fileName){
    std::string path = repoPath + fileName;
    path.replace(path.begin(), path.begin() + 36, Server::parentPath);
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

#pragma warning(push)
#pragma warning(disable:4267) // suppress warning for conversion from long long to size_t
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
#pragma warning(pop)

bool Server::addContributor(QString repoName, QString email, QString permission){

    int repositoryID;
    int userID;

    QSqlQuery repoID(Server::database);
    repoID.prepare("SELECT \"ID\" FROM public.\"Repositories\" WHERE \"Name\" = '" + repoName + "';");
    if (repoID.exec()){
        while (repoID.next()){
            repositoryID = repoID.value(0).toInt();
        }
    }

    QSqlQuery uID(Server::database);
    uID.prepare("SELECT \"ID\" FROM public.\"Users\" WHERE \"Email\" = '" + email + "';");
    if (uID.exec()){
        while (uID.next()){
            userID = uID.value(0).toInt();
        }
    }
    else
        return false;

    int ID = 0;
    QSqlQuery nextID(Server::database);
    nextID.prepare("SELECT Count(*) FROM public.\"RepoContributors\"");
    nextID.exec();
    if(nextID.next())
        ID = nextID.value(0).toInt();

    // Create new contributor
    QSqlQuery addContrib(Server::database);
    addContrib.prepare("INSERT INTO public.\"RepoContributors\" ("
                  "\"ID\","
                  "\"UserID\","
                  "\"RepositoryID\","
                  "\"Permission\")"
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
    findUser.prepare("Select \"ID\" from public.\"Users\" WHERE \"Email\" = '" + email + "';");
    if(findUser.exec())
        while(findUser.next())
            uID = findUser.value(0).toString();


    // remove user from repoContrib
    QSqlQuery remContrib(Server::database);
    remContrib.prepare("DELETE FROM public.\"RepoContributors\" WHERE \"UserID\" = " + uID + ";");

    if(!remContrib.exec())
        return false;

    // add reason for removal
    int ID = 0;
    QSqlQuery nextID(Server::database);
    nextID.prepare("SELECT MAX(\"ID\") FROM public.\"RemovalReasons\"");
    nextID.exec();
    if(nextID.next())
        ID = nextID.value(0).toInt() + 1;

    QSqlQuery remReason(Server::database);
    remReason.prepare("INSERT INTO public.\"RemovalReasons\"("
                      "\"ID\","
                      "\"Email\","
                      "\"Repository\","
                      "\"Reason\")"
                      "VALUES (?,?,?,?);");

    remReason.addBindValue(ID);
    remReason.addBindValue(uID);
    remReason.addBindValue(repoName);
    remReason.addBindValue(reason);

    if(!remReason.exec())
        return false;

    return true;

}

void Server::backupFiles(){
    while (true){
        for (auto& path : std::filesystem::recursive_directory_iterator(Server::parentPath)){
            if (path.is_regular_file()){

                // get file Name;
                std::string pathString = path.path().string();
                char* pathChar = &pathString[0];
                char* ptr = strrchr(pathChar, '\\');
                pathString.replace(pathString.begin(), pathString.begin() + (ptr - pathChar + 1), "");

                // get file data
                QSqlQuery getFileData(Server::database); // query to get file data
                getFileData.prepare("SELECT \"File\" FROM public.\"RepoFiles\" WHERE \"FileName\" = '" + QString::fromStdString(path.path().string()) + "';");
                getFileData.exec();
                while(getFileData.next()){
                    QFile file(QString::fromStdString(path.path().string())); // create file and save
                    file.open(QIODevice::WriteOnly);
                    file.write(getFileData.value(0).toByteArray());
                    file.close();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

bool Server::deleteRepo(int ID, std::string pass){

    QSqlQuery delRepo(Server::database);
    QSqlQuery delFiles(Server::database);
    QSqlQuery delDirectory(Server::database);
    QSqlQuery delContribs(Server::database);

    QByteArray passByteArr(pass.c_str(), pass.length());
    QString hashedPass = QString(QCryptographicHash::hash((passByteArr),QCryptographicHash::Md5).toHex());

    if(activeAdmin[4].toStdString() == hashedPass.toStdString()){
        delDirectory.prepare("SELECT \"Directory\" FROM public.\"Repositories\" WHERE \"ID\" = " + QString::number(ID) + ";");
        if (!delDirectory.exec()){
            qDebug()<< delDirectory.lastError();
            return false;
        }

        delDirectory.first();
        int result = std::filesystem::remove_all(delDirectory.value(0).toString().toStdString().c_str());
        if (result){
            delFiles.prepare("DELETE FROM public.\"RepoFiles\" WHERE \"RepoID\" = " + QString::number(ID) + ";");
            delContribs.prepare("DELETE FROM public.\"RepoContributors\" WHERE \"RepositoryID\" = " + QString::number(ID) + ";");

            if (!delFiles.exec() || !delContribs.exec()){
                qDebug() << delFiles.lastError();
                qDebug() << delContribs.lastError();
                return false;
            }

            delRepo.prepare("DELETE FROM public.\"Repositories\" WHERE \"ID\" = " + QString::number(ID) + ";");
            if (!delRepo.exec()){
                qDebug() << delRepo.lastError();
                return false;
            }
            return true;
        }
    }
    return false;
}

bool Server::isEmailValid(std::string email){
    const std::regex pattern("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
    return regex_match(email, pattern);
}

void Server::shutdown() {
    this->keepServerRunning = false;
    closesocket(this->server);
    WSACleanup();
}

bool Server::checkPassword(std::string password){

    bool uppercase = false;
    bool lowercase = false;
    bool symbol = false;
    bool number = false;
    int passwordLength = password.size();
    for (int i = 0; i < passwordLength; ++i) {
        if (std::isupper(password[i]))
            uppercase = true;
        if (std::islower(password[i]))
            lowercase = true;
        if (std::isdigit(password[i]))
            number = true;
        if (!std::isalnum(password[i]))
            symbol = true;
        if (uppercase && lowercase && symbol)
            break;
    }
    if (uppercase && lowercase && number && symbol && passwordLength >= 8)
        return true;
    return false;
}





