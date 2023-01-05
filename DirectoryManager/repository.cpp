#include "Repository.h"
#include "User.h"
#include <ctime>    // create date structure
#include <iostream>
#include <cstdlib>  // system()
#include <io.h>     // mkdir
#include <Windows.h>

Repository::Repository() {
    this->repoName = "";
    this->owner = 0;
    this->day = 0;
    this->month = 0;
    this->year = 0;
    this->directory = "";
    this->isPrivate = false;
}

Repository::Repository(const User& user, const std::string& name, bool isPriv) {

    // Create structure for current date
    time_t theTime = time(NULL);
    struct tm* aTime = localtime(&theTime);

    this->repoName = name;
    this->owner = user.getID();
    this->day = aTime->tm_mday;
    this->month = aTime->tm_mon;
    this->year = aTime->tm_year;
    this->directory = R"(C:\Users\karim\Desktop\Repositories\)" + name;
    this->isPrivate = isPriv;

    int isCreated = _wmkdir(std::wstring(this->directory.begin(), this->directory.end()).c_str());
    if (!isCreated) {
        std::cout << "Created New Repository: " << this->repoName << std::endl;
    }
    else {
        std::cout << "Failed To Create New Repository: " << this->repoName << std::endl;
    }

    // Repo creator is added to list of contributors and permissions
    this->getContributors().push_back(user.getID());
    this->getPermissions().push_back(3);
}

Repository::Repository(const User& user, const std::string& name, bool isPriv, const std::string& dir, std::vector<int> contribs, std::vector<int> perms, int d, int m, int y) {
    this->repoName = name;
    this->owner = user.getID();
    this->day = d;
    this->month = m;
    this->year = y;
    this->directory = dir;
    this->isPrivate = isPriv;
    this->contributors = contribs;
    this->permissions = perms;
}

// creates a directory
bool Repository::createDirectory(const std::string& dir) {
    std::string subDirectory = this->directory + "\\" + dir;
    int isCreated = _wmkdir(std::wstring(subDirectory.begin(), subDirectory.end()).c_str());
    if (!isCreated) {
        std::cout << "Created New Directory: " << dir << std::endl;
        return true;
    }
    else {
        std::cout << "Failed To Create New Directory: " << dir << std::endl;
        return false;
    }
}

// add as a file to a directory
void Repository::addFile(const std::string& fileName, std::string fileDirectory) {
    fileDirectory = fileDirectory + "\\" + fileName;
    std::ofstream newFile(fileDirectory);
    newFile.close();
}

// open and edit existing file
void Repository::editFile(const std::string& filePath) {
    std::cout << filePath << std::endl;
    system(filePath.c_str());
}

// delete file
void Repository::removeFile(std::string& filePath) {
    if (remove(filePath.c_str()) != 0) {
        std::cout << "Error deleting file" << std::endl;
    }
    else {
        std::cout << "File deleted." << std::endl;
    }
}

// add user to list of contributors and permissions for this repo
void Repository::addContributorToRepo(User& user, const int& userPermissions) {
    this->getContributors().push_back(user.getID());
    this->getPermissions().push_back(userPermissions);
}
