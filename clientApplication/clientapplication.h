#ifndef CLIENTAPPLICATION_H
#define CLIENTAPPLICATION_H

#include <QMainWindow>
#include <client.h>
#include <cstdlib>
#include <Windows.h>
#include <QMessageBox>
#include <QFileSystemModel>
#include <QListWidgetItem>
#include <fstream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QToolTip>
#include <thread>

QT_BEGIN_NAMESPACE
namespace Ui { class clientApplication; }
QT_END_NAMESPACE

class clientApplication : public QMainWindow
{
    Q_OBJECT

public:
    clientApplication(QWidget *parent = nullptr);
    ~clientApplication();

private slots:
    void on_singInButton_clicked();
    void on_singUpButton_clicked();
    void on_fileTree_customContextMenuRequested(const QPoint &pos);
    void on_createFolderButtons_accepted();
    void on_createFolderButtons_rejected();
    void on_actionEditFile_triggered();
    void on_actionNewTextFile_triggered();
    void on_deleteFileButton_clicked();
    void on_actionNewFolder_triggered();
    void on_createFileButtons_accepted();
    void on_createFileButtons_rejected();
    void on_actionNewWordDocument_triggered();
    void on_actionNewExcelSheet_triggered();
    void on_actionNewPowerPointPresentation_triggered();
    void on_repoList_itemClicked(QListWidgetItem *item);
    void on_repoList_customContextMenuRequested(const QPoint &pos);
    void on_createRootFolderButtons_accepted();
    void on_actionNewRootFolder_triggered();
    void on_createRootFolderButtons_rejected();
    void on_actionViewFile_triggered();
    void on_actionRunExecutable_triggered();

    void on_deleteAllButtons_accepted();

    void on_deleteAllButtons_rejected();

    void on_signOutButton_clicked();

    void on_passField_textEdited(const QString &arg1);

private:
    Ui::clientApplication *ui;
    Client* client;
    void setUpTabs();
    void openFile();
    QString userEmail;
    std::vector<QString> userRepos;
    std::vector<QString> userPerms;
    QFileSystemModel* fileSystem;
    QString selectedFolder;
    QString extension;
    QString repositoryPath;
};
#endif // CLIENTAPPLICATION_H
