#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->db = QSqlDatabase::addDatabase("QSQLITE", "test");
    this->db.setDatabaseName("C:/Users/karim/Desktop/DMDB.sqlite");

    this->server = new Server();
    this->signedIn = false;
    this->setUpTabs();
    this->setUpTables();
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::on_adminSignInButton_clicked(){
    std::string username = ui->usernameField->text().toStdString();
    std::string password = ui->passwordField->text().toStdString();

    if (this->server->adminSignIn(username, password)) { // if user and pass exist
        ui->usernameLabel->setText(QString::fromStdString(username));
        ui->serverTabs->setCurrentIndex(1);
        this->signedIn = true;


        QSqlQuery repos(this->db);
        QString repoQuery = "SELECT Name FROM Repositories;";
        repos.prepare(repoQuery);
        if (repos.exec()){
            while (repos.next()){
                ui->repositoryComboBox->addItem(repos.value(0).toString());
            }
        }

        ui->serverTabs->setTabEnabled(1, true);
        ui->serverTabs->setTabEnabled(2, true);
        ui->serverTabs->setTabEnabled(3, true);
    }
}

void MainWindow::on_startServerButton_clicked(){
    std::thread startServer(&Server::run, this->server); // run server
    startServer.detach();
    ui->StatusLabel->setFont(QFont("Segoe", 16, QFont::Bold));
    ui->StatusLabel->setStyleSheet("QLabel { color : green; }");
    ui->StatusLabel->setText("Running");
}

void MainWindow::on_stopServerButton_clicked(){
    ui->StatusLabel->setFont(QFont ("Segoe", 16, QFont::Bold));
    ui->StatusLabel->setStyleSheet("QLabel { color : red; }");
    ui->StatusLabel->setText("Not Running");
    this->server->shutdown();
    this->server = new Server();
}

void MainWindow::on_createAccountButton_clicked(){
    std::string firstName = ui->fNameField->text().toStdString();
    std::string lastName = ui->lNameField->text().toStdString();
    std::string email = ui->emailField->text().toStdString();
    std::string password = ui->passField->text().toStdString();
    this->server->addAdmin(firstName, lastName, email, password);
}

void MainWindow::updateActiveUsers(){

//    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "test");
//    db.setDatabaseName("C:/Users/karim/Desktop/DMDB.sqlite");

    QSqlQuery allUsers(this->db);
    QString usersQuery = "SELECT * FROM Users;";

    if (this->db.open()){
        qDebug () << "openededede";
        allUsers.prepare(usersQuery);
        //prepare other queries here
    }

    while(true){

        ui->activeUsers->setRowCount(0);
        for (auto& c : Server::connectedClientsMap) {
            ui->activeUsers->insertRow(ui->activeUsers->rowCount());
            ui->activeUsers->setItem(ui->activeUsers->rowCount() - 1, 0, new QTableWidgetItem(c.second[0]));
            ui->activeUsers->setItem(ui->activeUsers->rowCount() - 1, 1, new QTableWidgetItem(c.second[1]));
            ui->activeUsers->setItem(ui->activeUsers->rowCount() - 1, 2, new QTableWidgetItem(c.second[2]));
        }

        ui->userInfo->setRowCount(0);
        if(allUsers.exec()){
            while(allUsers.next()){
                ui->userInfo->insertRow(ui->userInfo->rowCount());
                ui->userInfo->setItem(ui->userInfo->rowCount() - 1, 0, new QTableWidgetItem(allUsers.value(0).toString()));
                ui->userInfo->setItem(ui->userInfo->rowCount() - 1, 1, new QTableWidgetItem(allUsers.value(1).toString() + " " + allUsers.value(2).toString()));
                ui->userInfo->setItem(ui->userInfo->rowCount() - 1, 2, new QTableWidgetItem(allUsers.value(3).toString()));
            }
        }

//        ui->repositoryComboBox->clear();
//        if (repos.exec()){
//            while (repos.next()){
//                ui->repositoryComboBox->addItem(repos.value(0).toString());
//            }
//        }

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void MainWindow::setUpTabs(){
    // set up tabs
    ui->serverTabs->setTabText(0, "Account");
    ui->serverTabs->setTabText(1, "Server Control");
    ui->serverTabs->setTabText(2, "Users");
    ui->serverTabs->setTabText(3, "Repositories");
    ui->serverTabs->setTabEnabled(1, false);
    ui->serverTabs->setTabEnabled(2, false);
    ui->serverTabs->setTabEnabled(3, false);
    ui->serverTabs->setCurrentIndex(0);
}

void MainWindow::setUpTables(){

    // set up tables
    ui->userInfo->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->userInfo->verticalHeader()->setVisible(false);
    ui->userInfo->setColumnWidth(0, 50);
    ui->userInfo->setColumnWidth(1, 154);
    ui->userInfo->setColumnWidth(2, 255);

    ui->activeUsers->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->activeUsers->verticalHeader()->setVisible(false);
    ui->activeUsers->setColumnWidth(0, 50);
    ui->activeUsers->setColumnWidth(1, 154);
    ui->activeUsers->setColumnWidth(2, 255);

    ui->repoInfoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->repoInfoTable->verticalHeader()->setVisible(false);
    ui->repoInfoTable->setColumnWidth(0, 50);
    ui->repoInfoTable->setColumnWidth(1, 209);
    ui->repoInfoTable->setColumnWidth(2, 100);

//    ui->activeReposTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
//    ui->activeReposTable->verticalHeader()->setVisible(false);
//    ui->activeReposTable->setColumnWidth(0, 200);
//    ui->activeReposTable->setColumnWidth(1, 169);

    // thread to update tables
    std::thread updateUserTable(&MainWindow::updateActiveUsers, this);
    updateUserTable.detach();
}

void MainWindow::on_repositoryComboBox_activated(){

    ui->repoInfoTable->setRowCount(0);

    QString repoName = ui->repositoryComboBox->currentText();
    QSqlQuery isPrivate(Server::database);
    isPrivate.prepare("SELECT ID, isPrivate FROM Repositories WHERE Name = '" + repoName + "';");

    QSqlQuery userInfo(Server::database);
    QSqlQuery userName(Server::database);

    if(isPrivate.exec()){
        while(isPrivate.next()){
            if (isPrivate.value(1).toString().toStdString() == "1")
                ui->isPrivateLabel->setText(QString::fromStdString("Private"));
            else
                ui->isPrivateLabel->setText(QString::fromStdString("Public"));

            userInfo.prepare("SELECT UserID, Permission FROM RepoContributors WHERE RepositoryID = " + isPrivate.value(0).toString() + ";");
            if(userInfo.exec()){
                while (userInfo.next()) {
                    userName.prepare("SELECT ID, Email FROM Users WHERE ID = " + userInfo.value(0).toString() + ";");
                    if(userName.exec()){
                        while(userName.next()){
                            ui->repoInfoTable->insertRow(ui->repoInfoTable->rowCount());
                            ui->repoInfoTable->setItem(ui->repoInfoTable->rowCount() - 1, 0, new QTableWidgetItem(userName.value(0).toString()));
                            ui->repoInfoTable->setItem(ui->repoInfoTable->rowCount() - 1, 1, new QTableWidgetItem(userName.value(1).toString()));
                            ui->repoInfoTable->setItem(ui->repoInfoTable->rowCount() - 1, 2, new QTableWidgetItem(userInfo.value(1).toString()));
                        }
                    }
                }
            }
        }
    }
}


void MainWindow::on_addContribButton_clicked(){
    QString repoName = ui->repositoryComboBox->currentText();
    QString userEmail = ui->addContribEmail->text();
    QString permission = ui->addContribPerm->currentText();

    if (permission == "Read")
        permission = "1";
    else if (permission == "Read & Write")
        permission = "2";
    else
        permission = "3";


    bool result = Server::addContributor(repoName, userEmail, permission);

    if (result == true)
        QMessageBox::information(this, tr("Success"), tr("Added User to Repository") );
    else
        QMessageBox::information(this, tr("Error"), tr("Error Adding User to Repository") );
}

void MainWindow::on_remContribButton_clicked(){
    QString repoName = ui->repositoryComboBox->currentText();
    QString email = ui->remContribEmail->text();
    QString reason = ui->remContribReason->text();

    bool result = Server::removeContributor(repoName, email, reason);

    if (result == true)
        QMessageBox::information(this, tr("Success"), tr("User Removed from Repository") );
    else if (result == false)
        QMessageBox::information(this, tr("Error"), tr("Error Removing User from Repository") );
}



void MainWindow::on_createRepoButton_clicked(){
    std::string repoName = ui->RepoName->text().toStdString();
    bool isPublic = ui->publicCheckBox->isChecked();
    bool isPrivate = ui->privateCheckBox->isChecked();

    if((isPublic && isPrivate) || (!isPublic && !isPrivate))
        QMessageBox::information(this, tr("Error"), tr("Pick Either Public or Private") );

    else{

        if (!isPublic && isPrivate)
            isPublic = false;

        bool isCreated = Server::createRepository(repoName, isPublic);
        if (isCreated){
            QMessageBox::information(this, tr("Success"), tr("Repository Created") );
            ui->repositoryComboBox->addItem(QString::fromStdString(repoName));

        }
        else
            QMessageBox::information(this, tr("Error"), tr("Could not create Repository") );
    }
}

