#include <QApplication>

#include "crypto.h"
#include "startupdialog.h"

int main(int argc, char *argv[])
{
    Crypto::init();
    QApplication a(argc, argv);
    StartupDialog w;
    w.show();
    return a.exec();
}
