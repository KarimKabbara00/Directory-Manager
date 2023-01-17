#ifndef CLIENTWORKER_H
#define CLIENTWORKER_H

#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include "Server.h"

#include <algorithm>
#include <QtSql>
#include <QSqlDatabase>

class ClientWorker{
private:

    SOCKET client;
    boolean clientConnected, loggedIn, repoSelected;
    std::vector<QString> activeUser;
    std::vector<QString> activeRepository;
    std::string parentPath;
    int clientID; // for Server::connectedClients (map)

    QSqlDatabase clientDatabase;

    void processClientRequest(const std::string& request);
    bool processLogin(const std::string& request);
    std::vector<QString> findRepository(int repoID);
    std::vector<std::string> getFilesInRepo(int flag);
    std::string Receive();
    void Send(std::string message) const;

public:
    ClientWorker();
    ClientWorker(SOCKET c, int id);
    void run();
    std::vector<QString>& getActiveUser(){return this->activeUser;}
    std::vector<QString>& getActiveRepository(){return this->activeRepository;}

};

#endif // CLIENTWORKER_H
