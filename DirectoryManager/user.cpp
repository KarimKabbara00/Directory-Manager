#include "User.h"
#include "Repository.h"

User::User() {
    this->userID = 0;
    this->firstName = "";
    this->lastName = "";
    this->email = "";
    this->password = "";
}

User::User(int id, std::string fName, std::string lName, std::string userEmail, std::string pass, bool isAdmin) {
    this->userID = id;
    this->firstName = fName;
    this->lastName = lName;
    this->email = userEmail;
    this->password = pass;
    this->isAdmin = isAdmin;
}

User::User(int id, std::string fName, std::string lName, std::string userEmail, std::string pass, bool admin, std::vector<Repository> repos, std::vector<int> perms) {
    this->userID = id;
    this->firstName = fName;
    this->lastName = lName;
    this->email = userEmail;
    this->password = pass;
    this->isAdmin = admin;
    this->repoList = repos;
    this->permissions = perms;
}

Repository User::createRepository(User user, const std::string& name, bool isPrivate) {
    Repository newRepo(user, name, isPrivate);        // Create new repository

    // Add newRepo to this list of repos and permissions
    this->getRepoList().emplace_back(newRepo);
    this->getPermissionsList().emplace_back(3);

    return newRepo;
}

bool User::createDirectory(Repository& repo, const std::string& directory) {
    return repo.createDirectory(directory);
}

void User::addFile(Repository& repo, const std::string& fileName, std::string dir) {
    repo.addFile(fileName, dir);
}

void User::editFile(Repository& repo, std::string& filePath) {
    repo.editFile(filePath);
}

void User::removeFile(Repository& repo, std::string& filePath) {
    repo.removeFile(filePath);
}

void User::addRepo(Repository repo) {
    this->repoList.emplace_back(repo);
}

void User::addPerm(int perm) {
    this->permissions.emplace_back(perm);
}

/*
 *  add user and permissions to that repos list of users and permissions
 *  add repo to newContributor's list of repos
 *  add repo to newContributor's list of permissions
 */
void User::addContributorToRepo(Repository& repo, User& newContributor, const int& userPermissions) {
    repo.addContributorToRepo(newContributor, userPermissions);
    newContributor.getRepoList().push_back(repo);
    newContributor.getPermissionsList().push_back(userPermissions);
}
