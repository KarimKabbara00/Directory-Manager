#include "clientapplication.h"
#include "qtcpsocket.h"
#include "ui_clientapplication.h"
#include "folder_filter.cpp"

clientApplication::clientApplication(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::clientApplication)
{
    ui->setupUi(this);
    this->setUpTabs();

    ui->createFolderBox->hide();
    ui->createFileBox->hide();
    ui->createRootFolderBox->hide();

    ui->deleteFileButton->setDisabled(true);

    ui->fileTree->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->fileTree->hide();
    ui->repoList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->repoList->hide();

    this->client = new Client();
    this->client->run();
}

clientApplication::~clientApplication(){
    delete ui;
}

void clientApplication::on_singInButton_clicked(){

    // pull from text fields
    std::string email = ui->emailField->text().toStdString();
    std::string password = ui->passwordField->text().toStdString();

    // construct and send message
    std::string message = this->client->constructMessage({"1", email, password});
    this->client->Send(message);

    // deal with result
    std::string result = this->client->Receive();
    std::vector<std::string> loginStatus = this->client->deconstructMessage(result);

    if (loginStatus[0] == "true"){
        ui->clientTabs->setTabEnabled(1, true);
        this->userEmail = ui->emailField->text();
        ui->fileTree->show();
        ui->repoList->show();
    }
    else{
        QMessageBox::information(this, tr("Error"), tr("Invalid Username/Password") );
    }

    message = this->client->constructMessage({"1"});
    this->client->Send(message);

    std::string repos = this->client->Receive();
    std::vector repositories = this->client->deconstructMessage(repos);

    std::string repoRequest = "fileRequest|";
    for (int i = 0; i < repositories.size()-1; i+=2){
        ui->repoList->addItem(QString::fromStdString(repositories[i]));
        repoRequest += repositories[i] + "|";
        this->userRepos.emplace_back(QString::fromStdString(repositories[i]));
        this->userPerms.emplace_back(QString::fromStdString(repositories[i+1]));
    }
    this->client->Send(repoRequest);
}

void clientApplication::on_singUpButton_clicked(){
    std::string firstName = ui->fNameField->text().toStdString();
    std::string lastName = ui->lNameField->text().toStdString();
    std::string email = ui->emailField_2->text().toStdString();
    std::string password = ui->passField->text().toStdString();

    std::string message = this->client->constructMessage({"2", firstName, lastName, email, password});
    this->client->Send(message);

    // deal with result
    std::string signUpStatus = this->client->Receive();
    if (signUpStatus == "true"){
        QMessageBox::information(this, tr("Success"), tr("New Account Created") );
    }
    else{
        QMessageBox::information(this, tr("Error"), tr("Could Not Create Account") );
    }
}



void clientApplication::setUpTabs(){
    // set up tabs
    ui->clientTabs->setTabText(0, "Account");
    ui->clientTabs->setTabText(1, "Repository");
    ui->clientTabs->setTabEnabled(1, false);
    ui->clientTabs->setCurrentIndex(0);
}

// List View right click menu
void clientApplication::on_repoList_itemClicked(QListWidgetItem *item){
    QString repoName = ui->repoList->currentItem()->text();
    qDebug() << repoName;

    if (repoName != ""){
        for (int i = 0; i < this->userRepos.size(); i++) {
            if (this->userRepos[i] == repoName){
                this->client->activeRepo = this->userRepos[i];
                this->client->activeRepoPerm = this->userPerms[i];
            }
        }

        // create file system for selected repo
        QString rootPath = "C:\\Users\\karim\\Desktop\\Repositories\\" + repoName;

        this->fileSystem = new QFileSystemModel(this);
        this->fileSystem->setRootPath(rootPath);

        ui->fileTree->setModel(this->fileSystem);
        ui->fileTree->setRootIndex((this->fileSystem->index(rootPath)));

        if (this->client->activeRepoPerm == "2" || this->client->activeRepoPerm == "3"){
            ui->deleteFileButton->setDisabled(false);
        }
    }
    else{
        ui->deleteFileButton->setDisabled(true);
        ui->fileTree->setModel(nullptr);
    }
}
void clientApplication::on_repoList_customContextMenuRequested(const QPoint &pos){

    QModelIndex index = ui->repoList->currentIndex();
    this->selectedFolder = index.data(Qt::DisplayRole).toString();

    QMenu menu(this);
    menu.addAction(ui->actionNewRootFolder);
    menu.exec(ui->repoList->mapToGlobal(pos));
}
void clientApplication::on_actionNewRootFolder_triggered(){
    ui->createRootFolderBox->show();
}
void clientApplication::on_createRootFolderButtons_accepted(){
    std::string folderName = "\\" + ui->rootFolderNameField->text().toStdString();
    std::string message = this->client->constructMessage({"5", this->selectedFolder.toStdString() + folderName});
    this->client->Send(message);

    std::string result = this->client->Receive();
    if (result == "true"){
        ui->createRootFolderBox->hide();
        QMessageBox::information(this, tr("Success"), tr("Folder Created") );
    }
    else
        QMessageBox::information(this, tr("Error"), tr("Could not Create Folder") );
}
void clientApplication::on_createRootFolderButtons_rejected(){
    ui->createRootFolderBox->hide();
}


// Tree View right click menu
void clientApplication::on_fileTree_customContextMenuRequested(const QPoint &pos){

    QModelIndex index = ui->fileTree->currentIndex();
    this->selectedFolder = this->fileSystem->filePath(index);
    QMenu menu(this);

    if(std::filesystem::is_directory(this->selectedFolder.toStdString())){
        if (this->client->activeRepoPerm == "2"){
            menu.addAction(ui->actionNewTextFile);
            menu.addAction(ui->actionNewWordDocument);
            menu.addAction(ui->actionNewExcelSheet);
            menu.addAction(ui->actionNewPowerPointPresentation);
            menu.addAction(ui->actionNewFolder);
        }
        else if (this->client->activeRepoPerm == "3"){
            menu.addAction(ui->actionNewTextFile);
            menu.addAction(ui->actionNewWordDocument);
            menu.addAction(ui->actionNewExcelSheet);
            menu.addAction(ui->actionNewPowerPointPresentation);
            menu.addAction(ui->actionNewFolder);
        }
    }
    else if (std::filesystem::is_regular_file(this->selectedFolder.toStdString())){
        if (this->client->activeRepoPerm == "1"){
            menu.addAction(ui->actionViewFile);
        }
        else if (this->client->activeRepoPerm == "2"){
            menu.addAction(ui->actionEditFile);
        }
        else if (this->client->activeRepoPerm == "3"){
            menu.addAction(ui->actionEditFile);
        }
    }
    else{   // executable
        if (this->client->activeRepoPerm == "3"){
            menu.addAction(ui->actionRunExecutable);
        }

    }
    menu.exec(ui->fileTree->mapToGlobal(pos));
}

// Create Folder
void clientApplication::on_actionNewFolder_triggered(){
    ui->createFolderBox->show();
}
void clientApplication::on_createFolderButtons_accepted(){
    std::string folderName = "\\" + ui->folderNameField->text().toStdString();
    std::string message = this->client->constructMessage({"4", this->selectedFolder.toStdString(), folderName});
    this->client->Send(message);

    std::string result = this->client->Receive();
    if (result == "true"){
        ui->createFolderBox->hide();
        QMessageBox::information(this, tr("Success"), tr("Folder Created") );
    }
    else
        QMessageBox::information(this, tr("Error"), tr("Could not Create Folder") );}
void clientApplication::on_createFolderButtons_rejected(){
    ui->createFolderBox->hide();
}

// Create Files
void clientApplication::on_actionNewTextFile_triggered(){
    ui->createNewFileLabel->setText("New Text File");
    ui->createFileBox->show();
    this->extension = ".txt";
}
void clientApplication::on_actionNewWordDocument_triggered(){
    ui->createNewFileLabel->setText("New Word Document");
    ui->createFileBox->show();
    this->extension = ".docx";
}
void clientApplication::on_actionNewExcelSheet_triggered(){
    ui->createNewFileLabel->setText("New Excel Sheet");
    ui->createFileBox->show();
    this->extension = ".xlsx";
}
void clientApplication::on_actionNewPowerPointPresentation_triggered(){
    ui->createNewFileLabel->setText("PowerPoint Presentation");
    ui->createFileBox->show();
    this->extension = ".pptx";
}
void clientApplication::on_createFileButtons_accepted(){

   std::string fileName = this->selectedFolder.toStdString();
   std::string file = ui->fileNameField->text().toStdString();
   std::string repoName = ui->repoList->currentItem()->text().toStdString();

   std::string message = this->client->constructMessage({"2", fileName, file, this->extension.toStdString(), repoName});

   this->client->Send(message);

   std::string result = this->client->Receive();

   if (result == "true"){
       ui->createFileBox->hide();
       ui->createNewFileLabel->clear();
       QMessageBox::information(this, tr("Success"), tr("File Created") );
   }
   else
       QMessageBox::information(this, tr("Error"), tr("Could not Create File") );
}
void clientApplication::on_createFileButtons_rejected(){
    ui->createFileBox->hide();
    ui->createNewFileLabel->clear();
}

//Edit File
void clientApplication::on_actionEditFile_triggered(){
    std::string file = this->selectedFolder.toStdString();
    std::string message = this->client->constructMessage({"6", file});

    this->client->Send(message);
    std::string result = this->client->Receive();

    system(result.c_str());

}

//Delete File/Folder        ADD OPTION TO DELETE ALL CONTENTS OF FOLDER AT ONCE
void clientApplication::on_deleteFileButton_clicked(){
    QModelIndex index = ui->fileTree->currentIndex();
    std::string path = this->fileSystem->filePath(index).toStdString();

    std::string message = this->client->constructMessage({"3", path});
    this->client->Send(message);

    std::string result = this->client->Receive();
    if (result == "true")
        QMessageBox::information(this, tr("Success"), tr("File Deleted") );
    else if (result == "false")
        QMessageBox::information(this, tr("Error"), tr("Couldn't Delete File") );
    else
        QMessageBox::information(this, tr("Error"), tr("Please Select a File/Folder") );
}

// Open File Read Only
void clientApplication::on_actionViewFile_triggered(){

}

// Run EXE
void clientApplication::on_actionRunExecutable_triggered(){

}

