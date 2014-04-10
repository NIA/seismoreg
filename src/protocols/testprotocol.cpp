#include "testprotocol.h"
// TODO: remove this dependency.
#include "serialprotocol.h" // for SerialProtocol::generateTiestamps
#include <QTimer>
#include <QTime>
#include <qmath.h>

namespace {
    void disableTimer(QTimer * timer) {
        if(timer != NULL) {
            timer->disconnect();
            timer->stop();
        }
    }
    const double OMEGA1 = 2*M_PI/1000;
    const double OMEGA2 = OMEGA1/10;
    const double NOISE_VALUE = 0.1;
    const double PHASE_SHIFT = M_PI/8;

    const double DEFAULT_LATITUDE  = 45.033333;
    const double DEFAULT_LONGITUDE = 38.983333;
    const double DEFAULT_ALTITUDE  = 30.0;
}

TestProtocol::TestProtocol(int dataSize, int amp, QObject *parent) :
    Protocol(parent), dataSize(dataSize), amp(amp), dataTimer(NULL), checkADCTimer(NULL), checkGPSTimer(NULL)
{
    qsrand(QTime::currentTime().msec());
    checkADCTimer = new QTimer(this);
    checkGPSTimer = new QTimer(this);
    dataTimer = new QTimer(this);
}

QString TestProtocol::description() {
    return tr("Test protocol x%1@%2").arg(dataSize).arg(amp);
}

bool TestProtocol::open() {
    addState(Open);
    return true;
}

void TestProtocol::checkADC() {
    addState(ADCWaiting);
    checkADCTimer->setSingleShot(true);
    connect(checkADCTimer, &QTimer::timeout, [=](){
        addState(ADCReady);
        removeState(ADCWaiting);
        emit checkedADC(true);
    });
    checkADCTimer->start(1100);
}

void TestProtocol::checkGPS() {
    addState(GPSWaiting);
    checkGPSTimer->setSingleShot(true);
    connect(checkGPSTimer, &QTimer::timeout, [=](){
        addState(GPSReady);
        removeState(GPSWaiting);
        emit positionAvailable(DEFAULT_LATITUDE, DEFAULT_LONGITUDE, DEFAULT_ALTITUDE);
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
        TimeStampsVector t = SerialProtocol::generateTimeStamps(1000, dataSize);
        emit dataAvailable(t, generateRandom(t));
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

TestProtocol::~TestProtocol() {
    close();
}

DataVector TestProtocol::generateRandom(TimeStampsVector ts) {
    DataVector res(dataSize);
    for(int i = 0; i < dataSize; ++i) {
        for(unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
            double t = ts[i].toMSecsSinceEpoch();
            res[i].byChannel[ch] =
                    amp*qSin(OMEGA1*t + PHASE_SHIFT*ch)*qCos(OMEGA2*t + PHASE_SHIFT*ch) +
                    qrand()*NOISE_VALUE*amp/RAND_MAX;
        }
    }
    return res;
}
