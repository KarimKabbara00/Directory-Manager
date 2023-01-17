#include "mainwindow.h"
#include "ui_mainwindow.h"

#define HOST "" // HOST NAME HERE
#define PORT    // PORT NUMBER HERE
#define USER "" // DB USERNAME HERE
#define PASS "" // DB PASSWORD HERE
#define NAME "" // DB NAME HERE

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Directory Manager Administrator");

    this->db = QSqlDatabase::addDatabase("QPSQL", "UISQL");
    this->db.setHostName(HOST);
    this->db.setPort(PORT);
    this->db.setUserName(USER);
    this->db.setPassword(PASS);
    this->db.setDatabaseName(NAME);
    if (this->db.open()){
        qDebug() << "opened DB from UI";
    }
    else
        qDebug() << "Couldnt open DB from UI";

    this->signedIn = false;
    this->setUpTabs();
    this->setUpTables();
    ui->confirmDelRepoBox->hide();
    ui->passField->setEchoMode(QLineEdit::Password);
    ui->confirmPasswordField->setEchoMode(QLineEdit::Password);
    ui->passwordField->setEchoMode(QLineEdit::Password);

    this->server = new Server();    
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

        ui->repositoryComboBox->insertItem(0, "");
        QSqlQuery repos(this->db);
        QString repoQuery = "SELECT \"Name\" FROM public.\"Repositories\";";
        repos.prepare(repoQuery);
        if (repos.exec()){
            while (repos.next()){
                ui->repositoryComboBox->addItem(repos.value(0).toString());
            }
        }
        ui->serverTabs->setTabEnabled(0, false);
        ui->serverTabs->setTabEnabled(1, true);
        ui->serverTabs->setTabEnabled(2, true);
        ui->serverTabs->setTabEnabled(3, true);
        ui->usernameField->setText("");
        ui->passwordField->setText("");
    }
    else{
        QMessageBox::information(this, tr("Error"), tr("Invalid Username or Password.") );
    }
}

void MainWindow::on_signOutButton_clicked(){
    ui->serverTabs->setCurrentIndex(0);
    ui->serverTabs->setTabEnabled(0, true);
    ui->serverTabs->setTabEnabled(1, false);
    ui->serverTabs->setTabEnabled(2, false);
    ui->serverTabs->setTabEnabled(3, false);

    ui->fNameField->setText("");
    ui->lNameField->setText("");;
    ui->emailField->setText("");;
    ui->passField->setText("");;
    ui->confirmPasswordField->setText("");
    ui->RepoName->setText("");
    ui->remRepIDField->setText("");
    ui->remRepNameField->setText("");
    ui->remContribEmail->setText("");
    ui->addContribEmail->setText("");
}

void MainWindow::on_startServerButton_clicked(){
    std::thread startServer(&Server::run, this->server); // run server
    startServer.detach();
    ui->StatusLabel->setFont(QFont("Segoe", 16, QFont::Bold));
    ui->StatusLabel->setStyleSheet("QLabel { color : green; }");
    ui->StatusLabel->setText("Running");

    ui->StatusLabel_2->setFont(QFont("Segoe", 16, QFont::Bold));
    ui->StatusLabel_2->setStyleSheet("QLabel { color : green; }");
    ui->StatusLabel_2->setText("Running");
}

void MainWindow::on_stopServerButton_clicked(){
    ui->StatusLabel->setFont(QFont ("Segoe", 16, QFont::Bold));
    ui->StatusLabel->setStyleSheet("QLabel { color : red; }");
    ui->StatusLabel->setText("Not Running");

    ui->StatusLabel_2->setFont(QFont("Segoe", 16, QFont::Bold));
    ui->StatusLabel_2->setStyleSheet("QLabel { color : green; }");
    ui->StatusLabel_2->setText("Running");

    this->server->shutdown();
    this->server = new Server();
}

void MainWindow::on_createAccountButton_clicked(){
    std::string firstName = ui->fNameField->text().toStdString();
    std::string lastName = ui->lNameField->text().toStdString();
    std::string email = ui->emailField->text().toStdString();
    std::string password = ui->passField->text().toStdString();
    bool result = this->server->addAdmin(firstName, lastName, email, password);
    if (result){
        QMessageBox::information(this, tr("Success"), tr("Created New Administrator") );
        ui->fNameField->setText("");
        ui->lNameField->setText("");;
        ui->emailField->setText("");;
        ui->passField->setText("");;
    }

    else
        QMessageBox::information(this, tr("Error"), tr("Error Creating New Administrator") );
}

void MainWindow::updateActiveUsers(){

    QSqlQuery allUsers(this->db);
    QString usersQuery = "SELECT * FROM public.\"Users\";";

    if (this->db.open()){
        allUsers.prepare(usersQuery);
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

    // thread to update tables
    std::thread updateUserTable(&MainWindow::updateActiveUsers, this);
    updateUserTable.detach();
}

void MainWindow::on_repositoryComboBox_activated(){

    ui->repoInfoTable->setRowCount(0);

    QString repoName = ui->repositoryComboBox->currentText();

    if (repoName != ""){
        QSqlQuery isPrivate(Server::database);
        isPrivate.prepare("SELECT \"ID\", \"isPrivate\" FROM public.\"Repositories\" WHERE \"Name\" = '" + repoName + "';");

        QSqlQuery userInfo(Server::database);
        QSqlQuery userName(Server::database);

        if(isPrivate.exec()){
            while(isPrivate.next()){
                if (isPrivate.value(1).toString().toStdString() == "true")
                    ui->isPrivateLabel->setText(QString::fromStdString("Private"));
                else
                    ui->isPrivateLabel->setText(QString::fromStdString("Public"));

                ui->IDLabel->setText("ID: " + isPrivate.value(0).toString());

                userInfo.prepare("SELECT \"UserID\", \"Permission\" FROM public.\"RepoContributors\" WHERE \"RepositoryID\" = " + isPrivate.value(0).toString() + ";");
                if(userInfo.exec()){
                    while (userInfo.next()) {
                        userName.prepare("SELECT \"ID\", \"Email\" FROM public.\"Users\" WHERE \"ID\" = " + userInfo.value(0).toString() + ";");
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
    else{
        ui->isPrivateLabel->setText("");
        ui->IDLabel->setText("");
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
    else if (permission == "Execute")
        permission = "3";
    else
        QMessageBox::information(this, tr("Error"), tr("Select a Permission Level") );

    if (permission != ""){
        bool result = Server::addContributor(repoName, userEmail, permission);

        if (result == true)
            QMessageBox::information(this, tr("Success"), tr("Added User to Repository") );
        else
            QMessageBox::information(this, tr("Error"), tr("Error Adding User to Repository") );

        ui->addContribEmail->setText("");
        ui->addContribPerm->setCurrentIndex(0);
    }
}

void MainWindow::on_remContribButton_clicked(){
    QString repoName = ui->repositoryComboBox->currentText();
    QString email = ui->remContribEmail->text();
    QString reason = ui->remContribReason->currentText();

    if (reason != ""){
        bool result = Server::removeContributor(repoName, email, reason);

        if (result == true)
            QMessageBox::information(this, tr("Success"), tr("User Removed from Repository") );
        else if (result == false)
            QMessageBox::information(this, tr("Error"), tr("Error Removing User from Repository") );

        ui->remContribEmail->setText("");
        ui->remContribReason->setCurrentIndex(0);
    }
    else
        QMessageBox::information(this, tr("Error"), tr("Select a Reason") );
}

void MainWindow::on_createRepoButton_clicked(){
    std::string repoName = ui->RepoName->text().toStdString();
    bool isPublic = ui->publicCheckBox->isChecked();
    bool isPrivate = ui->privateCheckBox->isChecked();

    if((isPublic && isPrivate) || (!isPublic && !isPrivate))
        QMessageBox::information(this, tr("Error"), tr("Pick Either Public or Private") );

    else{

        bool isCreated = false;
        if (isPublic)
            isCreated = Server::createRepository(repoName, false);
        else if (isPrivate)
            isCreated = Server::createRepository(repoName, true);

        if (isCreated){
            QMessageBox::information(this, tr("Success"), tr("Repository Created") );
            ui->repositoryComboBox->addItem(QString::fromStdString(repoName));
        }
        else
            QMessageBox::information(this, tr("Error"), tr("Could not Create Repository") );

        ui->RepoName->setText("");
        ui->publicCheckBox->setChecked(false);
    }
}

void MainWindow::on_remRepButton_clicked(){

    if (ui->remRepIDField->text() != "" && ui->remRepNameField->text() != "")
        ui->confirmDelRepoBox->show();
    else
        QMessageBox::information(this, tr("Error"), tr("Missing Information") );
}

void MainWindow::on_deleteRepoButton_clicked(){
    std::string repoID = ui->remRepIDField->text().toStdString();
    std::string repoName = ui->remRepNameField->text().toStdString();
    std::string pass = ui->confirmPasswordField->text().toStdString();

    bool result = Server::deleteRepo(std::stoi(repoID), pass);
    if (result){
        QMessageBox::information(this, tr("Success"), tr("Repository Deleted") );

        ui->repositoryComboBox->clear();
        ui->repositoryComboBox->insertItem(0, "");
        QSqlQuery repos(this->db);
        QString repoQuery = "SELECT \"Name\" FROM public.\"Repositories\";";
        repos.prepare(repoQuery);
        if (repos.exec()){
            while (repos.next()){
                ui->repositoryComboBox->addItem(repos.value(0).toString());
            }
        }
    }
    else
        QMessageBox::information(this, tr("Error"), tr("Could not delete Repository") );

    ui->remRepIDField->setText("");
    ui->remRepNameField->setText("");
    ui->confirmPasswordField->setText("");
    ui->confirmDelRepoBox->hide();
}

void MainWindow::on_cancelDelRepoButton_clicked(){
    ui->remRepIDField->setText("");
    ui->remRepNameField->setText("");
    ui->confirmPasswordField->setText("");
    ui->confirmDelRepoBox->hide();
}


void MainWindow::on_passField_textEdited(const QString &arg1){
    QToolTip::showText(ui->passField->mapToGlobal(QPoint()), "Password must meet the following criteria:"
                                                             "\n\t- An uppercase letter\n\t- A lowercase letter"
                                                             "\n\t- A number\n\t- A Symbol\n\t- At least 8 characters");
}

