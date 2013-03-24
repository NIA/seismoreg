#include "serialprotocol.h"
#include "qextserialport.h"
#include "qextserialenumerator.h"
#include "../logger.h"
#include <QTimer>
#include <QTime>
#include <QByteArray>

namespace {
    const QByteArray CHECK_ADC = "\03";
    const QByteArray CHECK_GPS = "\04";
    const QByteArray CHECKED_ADC = CHECK_ADC;
    const QByteArray CHECKED_GPS = CHECK_GPS;
    const QByteArray DATA_PREFIX(5, '\05');
}

SerialProtocol::SerialProtocol(QString portName, QObject *parent) :
    Protocol(parent), portName(portName), port(NULL)
{
    port = new QextSerialPort(portName);
    port->setBaudRate(BAUD57600);
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
        Logger::error(tr("Failed to open serial port %1").arg(portName));
        return false;
    }
}

void SerialProtocol::checkADC() {
    port->write("\03");
}

void SerialProtocol::checkGPS() {
    port->write("\04");
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
    // FIXME: send command
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
    if(hasState(Receiving) && rawData.startsWith(DATA_PREFIX)) {
        int dataSize = rawData.size() - DATA_PREFIX.size();
        // truncate bytes not fitting into a factor of sizeof(DataType)
        dataSize -= dataSize % sizeof(DataType);
        int dataCount = dataSize / sizeof(DataType);
        // allocate space for data array
        DataVector data(dataCount);
        // copy dataSize bytes to data array
        memcpy(data.data(), rawData.constData() + DATA_PREFIX.size(), dataSize);
        emit dataAvailable(data);
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
        names << port.portName;
    }
    return names;
}
