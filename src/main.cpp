#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QDebug>
#include <exception>

// Declare metatypes:
// TODO: better place for these?
#include "protocol.h"
#include "logger.h"
#include "worker.h"
Q_DECLARE_METATYPE(TimeStampsVector)
Q_DECLARE_METATYPE(DataVector)
Q_DECLARE_METATYPE(Logger::Level)
Q_DECLARE_METATYPE(Worker::PrepareResult)

int main(int argc, char *argv[])
{
    try {
    QApplication a(argc, argv);

    qRegisterMetaType<TimeStampsVector>("TimeStampsVector");
    qRegisterMetaType<DataVector>("DataVector");
    // TODO: should do something to correctly pass log messages between threads
    qRegisterMetaType<Logger::Level>("Level");
    qRegisterMetaType<Worker::PrepareResult>("PrepareResult");

    QTranslator translator, qtTranslator;
    translator.load("seismoreg_" + QLocale::system().name());
    qtTranslator.load("qtbase_" + QLocale::system().name());
    a.installTranslator(&translator);
    a.installTranslator(&qtTranslator);

    MainWindow w;
    w.show();
    
    return a.exec();
    } catch (const std::exception & e) {
        qCritical() << "BOOM! Exception caught: " << e.what();
        return 1;
    }
}
