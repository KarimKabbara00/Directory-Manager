#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Server.h"
#include <thread>
#include <QStringListModel>
#include <QMessageBox>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_createAccountButton_clicked();
    void on_startServerButton_clicked();
    void on_stopServerButton_clicked();
    void on_adminSignInButton_clicked();
    void on_repositoryComboBox_activated();
    void on_addContribButton_clicked();
    void on_remContribButton_clicked();

    void on_createRepoButton_clicked();

private:
    Ui::MainWindow *ui;
    bool signedIn;
    Server* server;
    void updateActiveUsers();
    void setUpTabs();
    void setUpTables();

    QSqlDatabase db;
};
#endif // MAINWINDOW_H
