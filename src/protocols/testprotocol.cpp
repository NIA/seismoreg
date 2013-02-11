#include "testprotocol.h"
#include <QTimer>
#include <QTime>

TestProtocol::TestProtocol(int dataSize, int mean, QObject *parent) :
    Protocol(parent), dataSize(dataSize), mean(mean), dataTimer(NULL)
{
    qsrand(QTime::currentTime().msec());
}

bool TestProtocol::open() {
    addState(Open);
    return true;
}

void TestProtocol::checkADC() {
    auto timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [=](){
        addState(ADCReady);
        emit checkedADC(true);
    });
    timer->start(1100);
}

void TestProtocol::checkGPS() {
    auto timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [=](){
        addState(GPSReady);
        emit checkedGPS(true);
    });
    timer->start(2000);
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
    dataTimer = new QTimer(this);
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
    dataTimer->stop();
    removeState(Receiving);
}

QVector<DataType> TestProtocol::generateRandom() {
    QVector<DataType> res(dataSize);
    for(int i = 0; i < dataSize; ++i) {
        res[i] = mean + qrand()*mean*0.1/RAND_MAX;
    }
    return res;
}
