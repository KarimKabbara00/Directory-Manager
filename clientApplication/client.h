#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <vector>
#include <ws2tcpip.h>
#include "inetpton4.h"
#include <fstream>
#include <QString>
#include <filesystem>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QFile>
#include <regex>

#pragma comment(lib, "ws2_32.lib")

class Client {
private:
    WSAData wsaData;
    SOCKADDR_IN address;
    SOCKET server;
    int portNumber;
    const char *hostIP;
    bool isConnected;

    void connectToServer();
    void closeSocket();

public:
    QString activeRepo;
    QString activeRepoPerm;
    QSqlDatabase clientDatabase;

    Client();
    void run();
    std::string Receive();
    void Send(std::string message) const;

    std::string constructMessage(std::initializer_list<std::string> il);
    std::vector<std::string> deconstructMessage(std::string message);

    bool makeDir(std::string dirPath);
    void makeFile(std::string fileName, std::string filePath, std::string extension, std::string repoName);
    void deleteFile(std::string path, std::string fileName);
    bool updateFile(std::string filePath);
    bool loadFiles(std::string fileNames, std::string filePaths);
};

#endif // CLIENT_H
