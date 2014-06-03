#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QDebug>
#include <exception>

// Declare metatypes:
// TODO: better place for these?
#include "protocol.h"
#include "logger.h"
Q_DECLARE_METATYPE(TimeStampsVector)
Q_DECLARE_METATYPE(DataVector)
//Q_DECLARE_METATYPE(Logger::Level)

int main(int argc, char *argv[])
{
    try {
    QApplication a(argc, argv);

    qRegisterMetaType<TimeStampsVector>("TimeStampsVector");
    qRegisterMetaType<DataVector>("DataVector");
    // TODO: should do something to correctly pass log messages between threads
    //qRegisterMetaType<Logger::Level>("Level");

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
