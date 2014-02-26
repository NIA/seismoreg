#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QDebug>
#include <exception>

int main(int argc, char *argv[])
{
    try {
    QApplication a(argc, argv);

    QTranslator translator;
    translator.load("seismoreg_" + QLocale::system().name());
    a.installTranslator(&translator);

    MainWindow w;
    w.show();
    
    return a.exec();
    } catch (const std::exception & e) {
        qCritical() << "BOOM! Exception caught: " << e.what();
        return 1;
    }
}
