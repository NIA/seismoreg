# This is Qt5 project! It will not work with Qt4
QT       += core gui widgets svg
# This project uses C+11 features! It requires GCC 4.7
CONFIG += c++11

TARGET = seismoreg
TEMPLATE = app

include(3rdparty/qextserialport/src/qextserialport.pri)
include(3rdparty/qwt/src/qwt.pri)

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/protocols/testprotocol.cpp \
    src/gui/qled/qled.cpp \
    src/worker.cpp \
    src/gui/logwindow.cpp \
    src/logger.cpp \
    src/protocols/serialprotocol.cpp \
    src/filewriter.cpp

HEADERS  += src/mainwindow.h \
    src/protocol.h \
    src/protocols/testprotocol.h \
    src/gui/qled/qled.h \
    src/worker.h \
    src/gui/logwindow.h \
    src/logger.h \
    src/protocols/serialprotocol.h \
    src/filewriter.h

FORMS    += mainwindow.ui

TRANSLATIONS += seismoreg_ru.ts

RESOURCES += \
    qled.qrc
