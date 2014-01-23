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
    const QByteArray START_RECEIVE = "\x01";
    const QByteArray CHECKED_ADC = CHECK_ADC;
    const QByteArray CHECKED_GPS = CHECK_GPS;
    const QByteArray DATA_PREFIX(5, '\xF0');

    const int MIN_PACKET_SIZE = 1;
}

SerialProtocol::SerialProtocol(QString portName, int packetSize, QObject *parent) :
    Protocol(parent), portName(portName), port(NULL), pointsInPacket(packetSize)
{
    port = new QextSerialPort(portName);
    port->setBaudRate(BAUD57600);
    if(pointsInPacket < MIN_PACKET_SIZE) {
        pointsInPacket = MIN_PACKET_SIZE;
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
    port->write(START_RECEIVE);
    addState(Receiving);
}

void SerialProtocol::stopReceiving() {
    if(! hasState(Receiving) ) {
        // TODO: report warning: already stopped
        return;
    }
    // FIXME: send command
    removeState(Receiving);
}

void SerialProtocol::close() {
    port->close();
}

void SerialProtocol::onDataReceived() {
    QByteArray rawData = port->readAll();
    if(hasState(Receiving)) {
        if(rawData.startsWith(DATA_PREFIX)) {
            // If we see packet start:
            // Drop buffer
            buffer.clear();
            // Remove prefix
            rawData.remove(0, DATA_PREFIX.size()); // TODO: optimize?
        }
        // Add data to buffer
        buffer += rawData;
        // if there is enough data in buffer to form and unwrap a packet, make it
        const int packetSize = pointsInPacket*sizeof(DataType);
        while(buffer.size() >= packetSize) {
            // allocate space for data array
            DataVector packetData(pointsInPacket);
            // copy packetSize bytes to data array (unwrap)
            memcpy(packetData.data(), buffer.constData(), packetSize);
            // remove them from buffer
            buffer.remove(0, packetSize);
            // notify
            emit dataAvailable(packetData);
        }
    } else {
        if(rawData.startsWith(CHECKED_ADC)) {
            addState(ADCReady);
            emit checkedADC(true);
        } else if(rawData.startsWith(CHECKED_GPS)) {
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
