#include "ClientWorker.h"

ClientWorker::ClientWorker(){}

ClientWorker::ClientWorker(SOCKET c, int id){
    this->client = c;
    this->clientConnected = true;
    this->repoSelected = false;
    this->loggedIn = false;
    this->parentPath = R"(C:\Users\karim\Desktop\Repositories\)";
    this->clientID = id;

    this->clientDatabase = QSqlDatabase::addDatabase("QSQLITE", QString::number(this->clientID));
    this->clientDatabase.setDatabaseName("C:/Users/karim/Desktop/DMDB.sqlite");
    if (this->clientDatabase.open()){
        qDebug() << "opened DB from client worker";
    }
    else
        qDebug() << "not opened DB from client worker";

}


void ClientWorker::run(){
    while (this->clientConnected) {
        // Authenticate
        while (!this->loggedIn) {
            this->loggedIn = this->processLogin(this->Receive());
        }
        // Process requests
        while (this->loggedIn) {
            this->processClientRequest(this->Receive());
        }
    }
}

bool ClientWorker::processLogin(const std::string& request) {

    std::vector<std::string> requestVector = Server::deconstructMessage(request);
    if (requestVector[0] == "1") {

        QString username = QString::fromStdString(requestVector[1]);
        QString password = QString::fromStdString(requestVector[2]);

        std::vector<std::string> repoNames; // stored in a vector to remove duplicates after querying
                                            // repositories and repoContributors table.

        //Authenticate
        QSqlQuery login(this->clientDatabase);
        QString loginQuery = "SELECT * FROM Users WHERE Email = '" + username + "' AND Password = '" + password + "';";
        login.prepare(loginQuery);
        if (login.exec()){
            while (login.next()){
                if(login.value(3) == username && login.value(4) == password){
                    this->activeUser.emplace_back(login.value(0).toString());   // id
                    this->activeUser.emplace_back(login.value(1).toString());   // first
                    this->activeUser.emplace_back(login.value(2).toString());   // last
                    this->activeUser.emplace_back(login.value(3).toString());   // email
                    this->activeUser.emplace_back(login.value(5).toString());   // isAdmin
                    Server::connectedClientsMap.emplace(this->clientID, this->activeUser);

                    this->Send("true|");
                    return true;
                }
            }
        }
        this->Send("false");
        return false;
    }

    else if (requestVector[0] == "2") {
        // send new account form

        QString fName = QString::fromStdString(requestVector[1]);
        QString lName = QString::fromStdString(requestVector[2]);
        QString email = QString::fromStdString(requestVector[3]);
        QString pass = QString::fromStdString(requestVector[4]);

        int ID = 0;
        QSqlQuery nextID(this->clientDatabase);
        nextID.prepare("SELECT Count(*) FROM Users");
        nextID.exec();
        if(nextID.next())
            ID = nextID.value(0).toInt();

        QSqlQuery signup(this->clientDatabase);
        signup.prepare("INSERT INTO Users ("
                      "ID,"
                      "firstName,"
                      "lastName,"
                      "Email,"
                      "Password,"
                      "isAdmin)"
                      "VALUES (?,?,?,?,?,?);");
        signup.addBindValue(ID);
        signup.addBindValue(fName);
        signup.addBindValue(lName);
        signup.addBindValue(email);
        signup.addBindValue(pass);
        signup.addBindValue(0);

        if(!signup.exec())
            this->Send("false");
        else{
            this->Send("true");
        }
    }
    return false;
}

void ClientWorker::processClientRequest(const std::string& request) {

    std::vector<std::string> requestVector = Server::deconstructMessage(request);

    if (requestVector[0] == "fileRequest"){
        for (int i = 1; i < requestVector.size(); ++i) {

        }
    }

    else if (requestVector[0] == "1") { // list repos for logged in user
        QSqlQuery getRepoIDs(Server::database);
        QSqlQuery getRepoNames(Server::database);
        std::string repoList;
        getRepoIDs.prepare("SELECT RepositoryID, Permission FROM RepoContributors WHERE UserID = " + this->activeUser[0] + ";");
        if(getRepoIDs.exec()){
            while(getRepoIDs.next()){
                getRepoNames.prepare("SELECT Name FROM Repositories WHERE ID = " + getRepoIDs.value(0).toString() + ";");
                if(getRepoNames.exec()){
                    while(getRepoNames.next()){
                        repoList += (getRepoNames.value(0).toString().toStdString() + "|" + getRepoIDs.value(1).toString().toStdString() + "|");
                    }
                }
            }
        }
        this->Send(repoList);
    }

    else if (requestVector[0] == "2") { // create a file
        std::string folderName = requestVector[1] + "/";
        std::string file = requestVector[2] + requestVector[3];
        std::string repoName = requestVector[4];

        bool isCreated = Server::createFile(folderName, file, repoName);

        if (isCreated)
            this->Send("true");
        else
            this->Send("false");

    }

    else if (requestVector[0] == "3"){ // delete a file
        int result = 1;
        if(std::filesystem::is_regular_file(std::filesystem::path(requestVector[1]))){ // if file
            result = remove(requestVector[1].c_str());
            if (!result) // remove returns 0 (false) if file is deleted
                this->Send("true");
            else
                this->Send("false");
        }
        else if(std::filesystem::is_directory(std::filesystem::path(requestVector[1])) && // if directory and empty
                std::filesystem::is_empty(std::filesystem::path(requestVector[1]))){
            result = std::filesystem::remove_all(requestVector[1].c_str());
            if (result) // filesystem::remove_a;; returns true if file is deleted
                this->Send("true");
            else
                this->Send("false");
        }
        else
            this->Send("None");
    }

    else if (requestVector[0] == "4"){ // create a folder
        bool isCreated = Server::createFolder(requestVector[1], requestVector[2]);
        if (isCreated)
            this->Send("true");
        else
            this->Send("false");
    }

    else if (requestVector[0] == "5"){ // create a root folder
        bool isCreated = Server::createRootFolder(requestVector[1]);
        if (isCreated)
            this->Send("true");
        else
            this->Send("false");
    }

    else if (requestVector[0] == "6"){
        this->Send(requestVector[1]);
    }

}

// receive message
std::string ClientWorker::Receive() {
    char buffer[1024];
    std::string message;
    int n = recv(this->client, buffer, sizeof(buffer), 0);
    if (n == SOCKET_ERROR) {
        this->clientConnected = false;
        Server::connectedClientsMap.erase(this->clientID);
//        Server::activeRepos.erase(this->repoID);
        return "";
    }
    else {
        message.append(buffer, buffer + n);
        return message;
    }
}

void ClientWorker::Send(std::string message) const {
    send(this->client, message.data(), message.size(), 0);
}

bool ClientWorker::transmitFiles(){


    std::ifstream fin("C:\\Users\\karim\\Desktop\\w.doc", std::ios::binary);
    FILE *fd = fopen("C:\\Users\\karim\\Desktop\\w.doc", "rb");
    if (fin) {

        fin.seekg(0, fin.end);
        int file_size = fin.tellg();
        fin.seekg(0, fin.beg);

        char buffer[1024];
        this->Send(std::to_string(file_size));

        while (true){
            size_t read_count = fread(buffer, 1, sizeof(buffer) - 1, fd);
            if (read_count == 0)
                break;
            buffer[sizeof(buffer) - 1] = '\0';
            send(this->client, buffer, read_count, 0);
        }
        return true;
    }
    else{
        return false;
    }
}



























