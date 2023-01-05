#ifndef CLIENTWORKER_H
#define CLIENTWORKER_H

#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include "Repository.h"
#include "Server.h"
#include <algorithm>
#include <QtSql>
#include <QSqlDatabase>
#include <cstdio>
#include <cstring>
#include <QTcpSocket>

class ClientWorker{
private:

    SOCKET client;
    boolean clientConnected, loggedIn, repoSelected;
    std::vector<QString> activeUser;
    std::vector<QString> activeRepository;
    std::string parentPath;
    int clientID; // for Server::connectedClients (map)
    int repoID;   // for Server::activeRepos (map)

    QSqlDatabase clientDatabase;
    QTcpSocket *fileSocket;

    void processClientRequest(const std::string& request);
    bool processLogin(const std::string& request);
    std::vector<QString> findRepository(int repoID);
//    User& getUserByID(int id);
//    User& getUserByEmail(const std::string& email);
    std::vector<std::string> getFilesInRepo(int flag);
    std::string Receive();
    void Send(std::string message) const;

public:
    ClientWorker();
    ClientWorker(SOCKET c, int id);
    void run();
    std::vector<QString>& getActiveUser(){return this->activeUser;}
    std::vector<QString>& getActiveRepository(){return this->activeRepository;}

    bool transmitFiles();
};

#endif // CLIENTWORKER_H
