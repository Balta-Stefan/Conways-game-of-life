#include "mainwindow.h"
#include "canvas.h"

#include <QApplication>


/*
klasa canvas sadrzi sve metode potrebne za kalkulisanje boje piksela

*/


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
