#include "clientapplication.h"
#include <QApplication>
#include <abstractfilesystem.h>>


int main(int argc, char *argv[]){
    QApplication a(argc, argv);
    clientApplication w;
    w.show();
    return a.exec();
}
