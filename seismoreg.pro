# This is Qt5 project! It will not work with Qt4
QT       += core gui widgets
# This project uses C+11 features! It requires GCC 4.7
QMAKE_CXXFLAGS += -std=c++11

TARGET = seismoreg
TEMPLATE = app

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/protocols/testprotocol.cpp

HEADERS  += src/mainwindow.h \
    src/protocol.h \
    src/protocols/testprotocol.h

FORMS    += mainwindow.ui
