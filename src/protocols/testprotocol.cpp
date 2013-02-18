#include "testprotocol.h"
#include <QTimer>
#include <QTime>

namespace {
    void disableTimer(QTimer * timer) {
        if(timer != NULL) {
            timer->disconnect();
            timer->stop();
        }
    }
}

TestProtocol::TestProtocol(int dataSize, int mean, QObject *parent) :
    Protocol(parent), dataSize(dataSize), mean(mean), dataTimer(NULL), checkADCTimer(NULL), checkGPSTimer(NULL)
{
    qsrand(QTime::currentTime().msec());
    checkADCTimer = new QTimer(this);
    checkGPSTimer = new QTimer(this);
    dataTimer = new QTimer(this);
}

QString TestProtocol::description() {
    return tr("Test protocol x%1@%2").arg(dataSize).arg(mean);
}

bool TestProtocol::open() {
    addState(Open);
    return true;
}

void TestProtocol::checkADC() {
    checkADCTimer->setSingleShot(true);
    connect(checkADCTimer, &QTimer::timeout, [=](){
        addState(ADCReady);
        emit checkedADC(true);
    });
    checkADCTimer->start(1100);
}

void TestProtocol::checkGPS() {
    checkGPSTimer->setSingleShot(true);
    connect(checkGPSTimer, &QTimer::timeout, [=](){
        addState(GPSReady);
        emit checkedGPS(true);
    });
    checkGPSTimer->start(2000);
}

void TestProtocol::startReceiving() {
    // TODO: move this logic to Protocol too?
    if(! hasState(ADCReady) ) {
        // TODO: report error: ADC not ready
        return;
    }
    if( hasState(Receiving) ) {
        // TODO: report warning: already receiving
        return;
    }
    addState(Receiving);
    connect(dataTimer, &QTimer::timeout, [=](){
        emit dataAvailable(generateRandom());
    });
    dataTimer->start(1000);
}

void TestProtocol::stopReceiving() {
    if(! hasState(Receiving) ) {
        // TODO: report warning: already stopped
        return;
    }
    disableTimer(dataTimer);
    removeState(Receiving);
}

void TestProtocol::close() {
    disableTimer(dataTimer);
    disableTimer(checkADCTimer);
    disableTimer(checkGPSTimer);
    resetState();
}

DataVector TestProtocol::generateRandom() {
    DataVector res(dataSize);
    for(int i = 0; i < dataSize; ++i) {
        res[i] = mean + qrand()*mean*0.1/RAND_MAX;
    }
    return res;
}
