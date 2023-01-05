#ifndef REPOSITORY_H
#define REPOSITORY_H


#pragma once
#include <string>
#include <chrono>
#include <fstream>
#include "User.h"

#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QDataStream>
#include <iostream>
#include <vector>

class User; // forward declare user to be used in the Repository Constructor. Needed because compiler won't know what user is

class Repository {
private:
    std::string repoName;
    std::string directory;
    std::vector<int> contributors; // Contributors for this repository
    std::vector<int> permissions;   // Permissions of each contributor in this repo
    int owner;
    int day;
    int month;
    int year;
    bool isPrivate;

    bool createDirectory(const std::string& directory);
    void addFile(const std::string& fileName, std::string dir);
    void editFile(const std::string& filePath);
    void removeFile(std::string& filePath);
    void addContributorToRepo(User& user, const int& userPermissions);
public:
    Repository();
    Repository(const User& user, const std::string& name, bool isPriv);
    Repository(const User& user, const std::string& name, bool isPriv, const std::string& dir, std::vector<int> contribs, std::vector<int> perms, int d, int m, int y);
    friend class User; // User is a friend class to repository so that user can access methods defined here

    std::string getName() { return this->repoName; }
    int getOwner() const { return this->owner; }
    std::string getDirectory() { return this->directory; }
    int getDayCreated() const { return this->day; }
    int getMonthCreated() const { return this->month; }
    int getYearCreated() const { return this->year; }
    bool getIsPrivate() const { return this->isPrivate; }
    std::vector<int>& getContributors() { return this->contributors; }
    std::vector<int>& getPermissions() { return this->permissions; }
    std::string getDateCreated() {return std::to_string(month) +"/"+ std::to_string(day) +"/"+ std::to_string(year);}

};
QDataStream& operator<<(QDataStream& out, const Repository& repository); // overload << operator to serialize repo class
QDataStream& operator>>(QDataStream& in, Repository& repository); // overload << operator to serialize repo class


#endif // REPOSITORY_H
