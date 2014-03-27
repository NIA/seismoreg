#include "serialprotocol.h"
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
    const QByteArray DATA_PREFIX(5, '\xF0');
    const QByteArray GPS_PREFIX = "\x10";

    const int MIN_FREQUENCY = 1;
    const int POINTS_IN_PACKET = 200;
    const int DEFAULT_FILTER_FREQ = 200;
}

PortSettingsEx::PortSettingsEx(BaudRateType baudRate, DataBitsType dataBits, ParityType parity, StopBitsType stopBits, FlowType flowControl, long timeoutMillisec, bool debug)
    : PortSettings({baudRate, dataBits, parity, stopBits, flowControl, timeoutMillisec}),
      debug(debug)
{}


const PortSettingsEx SerialProtocol::DEFAULT_PORT_SETTINGS(BAUD115200, DATA_8, PAR_NONE, STOP_1, FLOW_OFF, 10, false);

SerialProtocol::SerialProtocol(QString portName, int samplingFreq, int filterFreq, PortSettingsEx settings, QObject *parent) :
    Protocol(parent), portName(portName), port(NULL), samplingFrequency(samplingFreq), filterFrequency(filterFreq), debugMode(settings.debug)
{
    port = new QextSerialPort(portName);
    port->setBaudRate(settings.BaudRate);
    port->setDataBits(settings.DataBits);
    port->setStopBits(settings.StopBits);
    port->setParity(settings.Parity);
    port->setFlowControl(settings.FlowControl);
    // TODO: support timeout setting?

    if (samplingFrequency < MIN_FREQUENCY) {
        Logger::error(tr("Incorrect frequency: cannot be less than %1").arg(MIN_FREQUENCY));
        samplingFrequency = MIN_FREQUENCY;
    }
    if (POINTS_IN_PACKET % samplingFrequency != 0) { // and also if frequency > POINTS_IN_PACKET
        Logger::error(tr("Incorrect frequency: should be a divisor of %1").arg(POINTS_IN_PACKET));
        samplingFrequency = POINTS_IN_PACKET;
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
    if (filterFrequency == DEFAULT_FILTER_FREQ) {
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

    if (debugMode) {
        Logger::info(portName + ": " + rawData.toHex());
    }

    if (hasState(Receiving)) {
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
            DataVector packetData(samplingFrequency);
            // Unwrap data:
            if (samplingFrequency == POINTS_IN_PACKET) {
                // Easy case: just copy
                memcpy(packetData.data(), buffer.constData(), packetSize);
            } else {
                // Hard case: compute average of each avgSize items into one point
                int avgSize = POINTS_IN_PACKET / samplingFrequency; // frequency must not be and must not be greater that POINTS_IN_PACKET: it is checked in constructor
                const DataItem* curItem = reinterpret_cast<const DataItem*>(buffer.constData());
                for (int i = 0; i < samplingFrequency; ++i) {
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
        } else if(rawData.startsWith(GPS_PREFIX)) {
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
