#include "clientapplication.h"
#include <QApplication>

int main(int argc, char *argv[]){
    QApplication a(argc, argv);
    clientApplication w;
    w.show();
    return a.exec();
}
