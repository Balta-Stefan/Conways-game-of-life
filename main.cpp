#include "mainwindow.h"
#include "LifeCalculator.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    LifeCalculator obj1;
    //obj1.funkcija();

    return a.exec();
}
