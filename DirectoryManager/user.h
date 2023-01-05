#ifndef USER_H
#define USER_H

#pragma once
#include <string>
#include <vector>

class Repository;

class User {
private:
    int userID;
    std::string firstName;
    std::string lastName;
    std::string email;
    std::string password;
    bool isAdmin;
    std::vector<Repository> repoList;
    std::vector<int> permissions;

public:
    User();
    User(int id, std::string fName, std::string lName, std::string userEmail, std::string pass, bool admin);
    User(int id, std::string fName, std::string lName, std::string userEmail, std::string pass, bool admin, std::vector<Repository> repos, std::vector<int> perms);
    Repository createRepository(User user, const std::string& name, bool isPrivate);
    bool createDirectory(Repository& repo, const std::string& directory);
    void addFile(Repository& repo, const std::string& fileName, std::string dir);
    //    void editFile(Repository& repo, std::string fileDirectory, const std::string& fileName);
    void editFile(Repository& repo, std::string& filePath);
    void removeFile(Repository& repo, std::string& filePath);
    void addContributorToRepo(Repository& repo, User& newContributor, const int& userPermissions);
    void addRepo(Repository repo);
    void addPerm(int perm);

    int getID() const { return this->userID; }
    std::string getFirstName() { return this->firstName; }
    std::string getLastName() { return this->lastName; }
    std::string getFullName() { return this->firstName + " " + this->lastName; }
    std::string getEmail() { return this->email; }
    std::string getPassword() { return this->password; }
    bool getIsAdmin() {return this->isAdmin;}
    std::vector<Repository>& getRepoList() { return this->repoList; }
    std::vector<int>& getPermissionsList() { return this->permissions; }
};


#endif // USER_H
