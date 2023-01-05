#ifndef SERVER_H
#define SERVER_H


#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include "Repository.h"
#include "ClientWorker.h"

#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QDataStream>
#include <QtSql>
#include <QSqlDatabase>
#include <QString>

class Server {
    friend class ClientWorker;
    friend class MainWindow;
private:

    static int activeUserCount;                                     // count of active users
    static int activeReposCount;                                    // count of repos being used
    static std::vector<Repository> repositoryList;                  // all repos
    static std::vector<User> userList;                              // all users
    static std::map<int, std::vector<QString>> connectedClientsMap;         // active users
    static std::map<int, std::map<User*, Repository*>> activeRepos; // active repos

    static std::string parentPath;
    static std::vector<QString> activeAdmin;

    WSAData wsaData;
    SOCKADDR_IN serverAddress, clientAddress;
    SOCKET server, client;
    int portNumber, clientAddressSize;
    boolean keepServerRunning;

    bool adminSignIn(std::string username, std::string password);
    void addAdmin(std::string fName, std::string lName, std::string email, std::string password);

    ///////////////////////////////////
    static void serializeRepositories();
    static void deserializeRepositories();
    static void serializeUsers();
    static void deserializeUsers(); // must load repos for users right after.
    void saveData();
    void loadData();
    /////////////////////////////////////
    static bool createRepository(std::string name, bool isPublic);
    static bool createFile(std::string repoPath, std::string fileName, std::string repoName);
    static bool createFolder(std::string repoPath, std::string fileName);
    static bool createRootFolder(std::string folderPath);

public:

    static QSqlDatabase database;

    Server();
    void run();
    void shutdown();
    static User getUserByID(int id);
    static std::string constructMessage(std::initializer_list<std::string> il);
    static std::vector<std::string> deconstructMessage(std::string message);
    static bool addContributor(QString repoName, QString email, QString permission);
    static bool removeContributor(QString repoName, QString email, QString reason);
};

#endif // SERVER_H
