#include "client.h"
#include "qdebug.h"
#include <QSqlError>

#define HOST "" // HOST NAME HERE
#define PORT    // PORT NUMBER HERE
#define USER "" // DB USERNAME HERE
#define PASS "" // DB PASSWORD HERE
#define NAME "" // DB NAME HERE

Client::Client() {
    this->hostIP = ""; // SERVER IP HERE
    this->portNumber = 5555;
    WSAStartup(MAKEWORD(2, 0), &this->wsaData);
    this->server = socket(AF_INET, SOCK_STREAM, 0);
    inet_pton4(this->hostIP, reinterpret_cast<char *>(&address.sin_addr.s_addr));
    this->address.sin_family = AF_INET;
    this->address.sin_port = htons(this->portNumber);

    this->clientDatabase = QSqlDatabase::addDatabase("QPSQL", "UISQL");
    this->clientDatabase.setHostName(HOST);
    this->clientDatabase.setPort(PORT);
    this->clientDatabase.setUserName(USER);
    this->clientDatabase.setPassword(PASS);
    this->clientDatabase.setDatabaseName(NAME);
    if (this->clientDatabase.open()){
        qDebug() << "opened DB from UI";
    }
    else
        qDebug() << "couldnt open DB from UI";
}

void Client::connectToServer() {
    if (connect(this->server, reinterpret_cast<SOCKADDR *>(&this->address), sizeof(this->address)) == 0){
        this->isConnected = true;
    }
}

void Client::run() {
    this->connectToServer();
}

void Client::closeSocket() {
    closesocket(this->server);
    WSACleanup();
    std::cout << "Disconnected From Server" << std::endl;
}

std::string Client::Receive() {
    char buffer[1024];
    std::string message;
    int n = recv(this->server, buffer, sizeof(buffer), 0);
    message.append(buffer, buffer + n);
    return message;
}

void Client::Send(std::string message) const {
    send(this->server, message.data(), message.size(), 0);
}

std::string Client::constructMessage(std::initializer_list<std::string> il){
    std::string message;
    for (auto const& x: il)
        message += (x + "|");
    return message;
}


std::vector<std::string> Client::deconstructMessage(std::string message){

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


bool Client::makeDir(std::string dirPath){
    std::wstring wpath = std::wstring(dirPath.begin(), dirPath.end());
    int isCreated = _wmkdir(wpath.c_str());
    if (!isCreated)
        return true;
    else
        return false;
}


void Client::makeFile(std::string fileName, std::string filePath, std::string extension, std::string repoName){

    std::string folderName = fileName + "/";
    std::string file = filePath + extension;

    std::ofstream newFile (folderName + file);
    newFile.close();

    // primary key RepoFiles
    int ID = 0;
    QSqlQuery nextID(this->clientDatabase);
    nextID.prepare("SELECT MAX(ID) FROM public.\"RepoFiles\"");
    nextID.exec();
    if(nextID.next())
        ID = nextID.value(0).toInt() + 1;

    // RepoID for RepoFiles
    int repositoryID;
    QSqlQuery repoID(this->clientDatabase);
    repoID.prepare("SELECT \"ID\" FROM public.\"Repositories\" WHERE \"Name\" = '" + QString::fromStdString(repoName) + "';");
    repoID.exec();
    if(repoID.next())
        repositoryID = repoID.value(0).toInt();


    // Save file to Database
    QSqlQuery addFile(this->clientDatabase);
    QFile fileByte(QString::fromStdString(filePath + fileName));
    fileByte.open(QIODeviceBase::ReadOnly);
    QByteArray fileData = fileByte.readAll();

    addFile.prepare("INSERT INTO public.\"RepoFiles\" ("
                    "\"ID\","
                    "\"RepoID\","
                    "\"FileName\","
                    "\"File\")"
                    "VALUES (?,?,?,?);");

    addFile.addBindValue(ID);
    addFile.addBindValue(repositoryID);
    addFile.addBindValue(QString::fromStdString(folderName + file));
    addFile.addBindValue(fileData);

    addFile.exec();

}

void Client::deleteFile(std::string path, std::string fileName){

    if(std::filesystem::is_regular_file(std::filesystem::path(path))){ // if file

        // remove file
        remove(path.c_str());

        // get file name
//        char* pathChar = &fileName[0];
//        char* ptr = strrchr(pathChar, '/');
//        fileName.replace(fileName.begin(), fileName.begin() + (ptr - pathChar + 1), "");

        //delete file from db
        QSqlQuery removeQuery(this->clientDatabase);
        removeQuery.prepare("DELETE FROM public.\"RepoFiles\" WHERE \"FileName\" = '" + QString::fromStdString(fileName) + "';");
        removeQuery.exec();
    }
    else if(std::filesystem::is_directory(std::filesystem::path(path)) && // if directory and empty
            std::filesystem::is_empty(std::filesystem::path(path))){
        std::filesystem::remove_all(path.c_str());
    }
    else if(std::filesystem::is_directory(std::filesystem::path(path)) && // if directory and empty
            !std::filesystem::is_empty(std::filesystem::path(path))){
        std::filesystem::remove_all(path.c_str());
    }
}

bool Client::updateFile(std::string filePath){
    QSqlQuery updateQuery(this->clientDatabase);
    QFile file(QString::fromStdString(filePath));
    file.open(QIODevice::ReadOnly);
    QByteArray fileData = file.readAll();

    // get file name
//    char* pathChar = &filePath[0];
//    char* ptr = strrchr(pathChar, '/');
//    filePath.replace(filePath.begin(), filePath.begin() + (ptr - pathChar + 1), "");

    // update file in db
    updateQuery.prepare("UPDATE public.\"RepoFiles\" SET \"File\" =:fileData WHERE \"FileName\" =:fileName ;");

    updateQuery.bindValue(":fileData", fileData);
    updateQuery.bindValue(":fileName", QString::fromStdString(filePath));

    if (!updateQuery.exec()){
        qDebug() << updateQuery.lastError();
        return false;
    }
    return true;
}

bool Client::loadFiles(std::string fileNames, std::string filePaths){
    std::vector<std::string> names = this->deconstructMessage(fileNames);
    std::vector<std::string> paths = this->deconstructMessage(filePaths);

    // get repo names
    this->Send("1");
    std::string repos = this->Receive();

    // get folders within repos
    this->Send("Folder Request|" + repos);

    qDebug() << QString::fromStdString("Folder Request|" + repos);

    std::string folderPaths = this->Receive();
    std::vector<std::string> folders = this->deconstructMessage(folderPaths);

    // make folders
    for (int i = 0; i < folders.size(); ++i){
        qDebug() << QString::fromStdString(folders[i]);
        std::wstring widePath = std::wstring(folders[i].begin(), folders[i].end());
        int isCreated = _wmkdir(widePath.c_str());
    }

    QSqlQuery getFileData(this->clientDatabase); // query to get file data
    for (int i = 0; i < names.size(); ++i) {    // loop through file names
        std::string fileName = std::regex_replace(paths[i], std::regex("\\\\"), "/");
        getFileData.prepare("SELECT \"RepoID\", \"File\" FROM public.\"RepoFiles\" WHERE \"FileName\" = '" + QString::fromStdString(fileName) + "';"); // prepare to find file id, and file data
        getFileData.exec(); // get file id/data
        qDebug() << "SELECT \"RepoID\", \"File\" FROM public.\"RepoFiles\" WHERE \"FileName\" = '" + QString::fromStdString(fileName) + "';";
        while(getFileData.next()){
            QByteArray fileData = getFileData.value(1).toByteArray(); // load file data into byte array
            QSqlQuery repoName(this->clientDatabase);   // query to get repo name
            repoName.prepare("SELECT \"Name\" FROM public.\"Repositories\" WHERE \"ID\" = " + getFileData.value(0).toString() + ";"); // prepare to get repo name using file id
            qDebug() << "SELECT \"Name\" FROM public.\"Repositories\" WHERE \"ID\" = " + getFileData.value(0).toString() + ";";
            if(!repoName.exec())
                return false;
            while (repoName.next()){
                QString path = QString::fromStdString(paths[i]);
                QFile file(path); // create file and save
                file.open(QIODevice::WriteOnly);
                file.write(fileData);
                file.close();
            }
        }
    }
    return true;
}






