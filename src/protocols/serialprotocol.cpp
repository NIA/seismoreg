#include "serialprotocol.h"
#include "qextserialport.h"
#include "qextserialenumerator.h"
#include "../logger.h"
#include <QTimer>
#include <QTime>
#include <QByteArray>

namespace {
    const QByteArray CHECK_ADC = "\x03";
    const QByteArray CHECK_GPS = "\x04";
    const QByteArray START_RECEIVE_50 = "\x01";
    const QByteArray START_RECEIVE_200 = "\x02";
    const QByteArray STOP_RECEIVE("\x00", 1); // simply = "\x00" won't work: will be empty string
    const QByteArray CHECKED_ADC = CHECK_ADC;
    const QByteArray CHECKED_GPS = CHECK_GPS;
    const QByteArray DATA_PREFIX(5, '\xF0');

    const int MIN_FREQUENCY = 1;
    const int POINTS_IN_PACKET = 200;
}

SerialProtocol::SerialProtocol(QString portName, int samplingFrequency, QObject *parent) :
    Protocol(parent), portName(portName), port(NULL), frequency(samplingFrequency)
{
    port = new QextSerialPort(portName);
    // TODO: configurable baud rate
    port->setBaudRate(BAUD115200);
    if (frequency < MIN_FREQUENCY) {
        Logger::error(tr("Incorrect frequency: cannot be less than %1").arg(MIN_FREQUENCY));
        frequency = MIN_FREQUENCY;
    }
    if (POINTS_IN_PACKET % frequency != 0) { // and also if frequency > POINTS_IN_PACKET
        Logger::error(tr("Incorrect frequency: should be a divisor of %1").arg(POINTS_IN_PACKET));
        frequency = POINTS_IN_PACKET;
    }
}

QString SerialProtocol::description() {
    return tr("Serial port %1").arg(portName);
}

bool SerialProtocol::open() {
    if(port->open(QextSerialPort::ReadWrite)) {
        addState(Open);
        connect(port, &QextSerialPort::readyRead, this, &SerialProtocol::onDataReceived);
        return true;
    } else {
        if(port->lastError() != E_NO_ERROR) {
            Logger::error(port->errorString());
        }
        return false;
    }
}

void SerialProtocol::checkADC() {
    port->write(CHECK_ADC);
}

void SerialProtocol::checkGPS() {
    port->write(CHECK_GPS);
}

void SerialProtocol::startReceiving() {
    // TODO: move this logic to Protocol too?
    if(! hasState(ADCReady) ) {
        // TODO: report error: ADC not ready
        return;
    }
    if( hasState(Receiving) ) {
        // TODO: report warning: already receiving
        return;
    }
    if (frequency == POINTS_IN_PACKET) {
        port->write(START_RECEIVE_200);
    } else {
        port->write(START_RECEIVE_50);
    }
    addState(Receiving);
}

void SerialProtocol::stopReceiving() {
    if(! hasState(Receiving) ) {
        // TODO: report warning: already stopped
        return;
    }
    port->write(STOP_RECEIVE);
    removeState(Receiving);
}

void SerialProtocol::close() {
    if (hasState(Receiving))  {
        stopReceiving();
    }
    port->close();
    resetState();
}

SerialProtocol::~SerialProtocol() {
    close();
}

void SerialProtocol::onDataReceived() {
    QByteArray rawData = port->readAll();
    if(hasState(Receiving)) {
        if(rawData.startsWith(DATA_PREFIX)) {
            // If we see packet start:
            // Drop buffer
            buffer.clear();
            // Remove prefix
            rawData.remove(0, DATA_PREFIX.size()); // TODO: optimize? (avoid removing from beginning here and below)
        }
        // Add data to buffer
        buffer += rawData;
        // if there is enough data in buffer to form and unwrap a packet, make it
        const int packetSize = CHANNELS_NUM*POINTS_IN_PACKET*sizeof(DataType);
        while(buffer.size() >= packetSize) {
            // allocate space for data array
            DataVector packetData(frequency);
            // Unwrap data:
            if (frequency == POINTS_IN_PACKET) {
                // Easy case: just copy
                memcpy(packetData.data(), buffer.constData(), packetSize);
            } else {
                // Hard case: compute average of each avgSize items into one point
                int avgSize = POINTS_IN_PACKET / frequency; // frequency must not be and must not be greater that POINTS_IN_PACKET: it is checked in constructor
                const DataItem* curItem = reinterpret_cast<const DataItem*>(buffer.constData());
                for (int i = 0; i < frequency; ++i) {
                    double sums[CHANNELS_NUM];
                    for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) { sums[ch] = 0; }
                    for (int j = 0; j < avgSize; ++j) {
                        for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
                            sums[ch] += curItem->byChannel[ch];
                        }
                        ++curItem;
                    }
                    for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
                        packetData[i].byChannel[ch] = sums[ch] / avgSize;
                    }
                }
            }
            // remove them from buffer
            buffer.remove(0, packetSize);
            // generate timestamps
            TimeStampsVector timeStamps = generateTimeStamps(1000, packetData.size());
            // notify
            emit dataAvailable(timeStamps, packetData);
        }
    } else {
        if(rawData.startsWith(CHECKED_ADC)) {
            addState(ADCReady);
            emit checkedADC(true);
            // FIXME: temporary workaround!!!
            addState(GPSReady);
            emit checkedGPS(true);
        } else if(rawData.startsWith(CHECKED_GPS)) {
            // FIXME: actual GPS packet parsing!!!
            addState(GPSReady);
            emit checkedGPS(true);
        }
    }
}

QList<QString> SerialProtocol::portNames() {
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
    QList<QString> names;
    foreach (QextPortInfo port, ports) {
        if( ! port.portName.isEmpty() ) {
            names << port.portName;
        }
    }
    return names;
}

TimeStampsVector SerialProtocol::generateTimeStamps(double periodMsecs, int count) {
    TimeStampsVector res(count);

    TimeStampType start = TimeStampType::currentDateTime().addMSecs(-periodMsecs);
    double deltaMsecs = periodMsecs / count;
    for (int i = 0; i < count; ++i) {
        res[i] = start.addMSecs( i*deltaMsecs );
    }
    return res;
}
