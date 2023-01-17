#include "ClientWorker.h"

#define HOST "" // HOST NAME HERE
#define PORT    // PORT NUMBER HERE
#define USER "" // DB USERNAME HERE
#define PASS "" // DB PASSWORD HERE
#define NAME "" // DB NAME HERE

ClientWorker::ClientWorker(){}

ClientWorker::ClientWorker(SOCKET c, int id){
    this->client = c;
    this->clientConnected = true;
    this->repoSelected = false;
    this->loggedIn = false;
    this->parentPath = ""; // PATH TO SERVER DIRECTORY
    this->clientID = id;

    this->clientDatabase = QSqlDatabase::addDatabase("QPSQL", QString::number(this->clientID));
    this->clientDatabase.setHostName(HOST);
    this->clientDatabase.setPort(PORT);
    this->clientDatabase.setUserName(USER);
    this->clientDatabase.setPassword(PASS);
    this->clientDatabase.setDatabaseName(NAME);
    if (this->clientDatabase.open()){
        qDebug() << "opened DB from client worker";
    }
    else
        qDebug() << "Couldnt open DB from client worker";

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
        QByteArray passByteArr(requestVector[2].c_str(), requestVector[2].length());
        QString hashedPass = QString(QCryptographicHash::hash((passByteArr),QCryptographicHash::Md5).toHex());

        std::vector<std::string> repoNames; // stored in a vector to remove duplicates after querying
                                            // repositories and repoContributors table.

        //Authenticate
        QSqlQuery login(this->clientDatabase);
        QString loginQuery = "SELECT * FROM public.\"Users\" WHERE \"Email\" = '" + username + "' AND \"Password\" = '" + hashedPass + "';";
        login.prepare(loginQuery);
        if (login.exec()){
            while (login.next()){
                if(login.value(3) == username && login.value(4) == hashedPass){
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

        fName[0] = fName[0].toUpper();
        lName[0] = lName[0].toUpper();

        if(!Server::isEmailValid(email.toStdString()) || !Server::checkPassword(requestVector[4])){
            this->Send("false");
            return false;
        }


        QByteArray passByteArr(requestVector[4].c_str(), requestVector[4].length());
        QString hashedPass = QString(QCryptographicHash::hash((passByteArr),QCryptographicHash::Md5).toHex());

        int ID = 0;
        QSqlQuery nextID(this->clientDatabase);
        nextID.prepare("SELECT Count(*) FROM public.\"Users\"");
        nextID.exec();
        if(nextID.next())
            ID = nextID.value(0).toInt();

        QSqlQuery signup(this->clientDatabase);
        signup.prepare("INSERT INTO public.\"Users\" ("
                      "\"ID\","
                      "\"firstName\","
                      "\"lastName\","
                      "\"Email\","
                      "\"Password\","
                      "\"isAdmin\")"
                      "VALUES (?,?,?,?,?,?);");
        signup.addBindValue(ID);
        signup.addBindValue(fName);
        signup.addBindValue(lName);
        signup.addBindValue(email);
        signup.addBindValue(hashedPass);
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
        std::string filePaths = "";
        std::string fileNames = "";

        // only send files. Folder structure on client side will not be deleted.
        for (int i = 1; i < requestVector.size()-1; ++i) {
            for (auto& path : std::filesystem::recursive_directory_iterator(Server::parentPath + requestVector[i])){
                if (path.is_regular_file()){
                    fileNames += (path.path().filename().string() + "|");
                    filePaths += (path.path().string() + "|");
                }
            }
        }
        if (fileNames == "" || filePaths == ""){
            this->Send("None");
            this->Send("None");
        }
        else{
            this->Send(fileNames);
            this->Send(filePaths);
        }
    }

    else if (requestVector[0] == "Folder Request") {
        std::string folderPaths = "";
        for (int i = 1; i < requestVector.size()-1; i+=2) {
            qDebug() << QString::fromStdString(Server::parentPath + requestVector[i]);
            for (auto& path : std::filesystem::recursive_directory_iterator(Server::parentPath + requestVector[i])){
                if (path.is_directory()){
                    folderPaths += (path.path().string() + "|");
                }
            }
            if (folderPaths == ""){
                this->Send("None");
            }
            else{
                this->Send(folderPaths);
            }
        }
    }

    else if (requestVector[0] == "1") { // list repos for logged in user
        QSqlQuery getRepoIDs(Server::database);
        QSqlQuery getRepoNames(Server::database);
        std::string repoList;

        // get private repos
        getRepoIDs.prepare("SELECT \"RepositoryID\", \"Permission\" FROM public.\"RepoContributors\" WHERE \"UserID\" = " + this->activeUser[0] + ";");
        if(getRepoIDs.exec()){
            while(getRepoIDs.next()){
                getRepoNames.prepare("SELECT \"Name\" FROM public.\"Repositories\" WHERE \"ID\" = " + getRepoIDs.value(0).toString() + ";");
                if(getRepoNames.exec()){
                    while(getRepoNames.next()){
                        repoList += (getRepoNames.value(0).toString().toStdString() + "|" + getRepoIDs.value(1).toString().toStdString() + "|");
                    }
                }
            }
        }

        // get public repos
        getRepoNames.prepare("SELECT \"Name\" FROM public.\"Repositories\" WHERE \"isPrivate\" = false;");
        if(getRepoNames.exec()){
            while(getRepoNames.next()){
                repoList += (getRepoNames.value(0).toString().toStdString() + "|" + "3" + "|");
            }
        }

        if (Server::deconstructMessage(repoList).size() <= 1)
            this->Send("None|");
        else
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
            if (!result){ // remove returns 0 (false) if file is deleted
                try {
                    QSqlQuery removeQuery(this->clientDatabase);
                    removeQuery.prepare("DELETE FROM public.\"RepoFiles\" WHERE \"FileName\" = '" + QString::fromStdString(requestVector[1]) + "';");
                    removeQuery.exec();

                    this->Send("true");
                }
                catch (...) {
                    this->Send("false");
                }
            }
            else
                this->Send("false");
        }
        else if(std::filesystem::is_directory(std::filesystem::path(requestVector[1])) && std::filesystem::is_empty(std::filesystem::path(requestVector[1]))){
            result = std::filesystem::remove_all(requestVector[1].c_str());
            if (result) // filesystem::remove_all returns true if file is deleted
                this->Send("true");
            else
                this->Send("false");
        }
        else if(std::filesystem::is_directory(std::filesystem::path(requestVector[1])) && !std::filesystem::is_empty(std::filesystem::path(requestVector[1]))){
            this->Send("Delete All");
            std::string confirmation = this->Receive();
            if (confirmation == "True"){
                std::vector<std::string> allFiles;
                for (auto& path : std::filesystem::recursive_directory_iterator(requestVector[1].c_str())){
                    if (path.is_regular_file()){
                        std::string fileName = path.path().string();
                        allFiles.emplace_back(fileName);
                    }
                }
                result = std::filesystem::remove_all(requestVector[1].c_str());
                if (result){ // filesystem::remove_all returns true if file is deleted
                    QSqlQuery removeQuery(this->clientDatabase);
                    for (std::string &file : allFiles) {
                        removeQuery.prepare("DELETE FROM public.\"RepoFiles\" WHERE \"FileName\" = '" + QString::fromStdString(file) + "';");
                        if(!removeQuery.exec())
                            this->Send("false");
                        this->Send("true");
                    }
                }
                else
                    this->Send("false");
            }
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
    else if (requestVector[0] == "7"){
        this->activeUser.clear();
        this->activeRepository.clear();
        this->loggedIn = false;
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



























