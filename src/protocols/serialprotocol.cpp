#include "serialprotocol.h"
#include "qextserialenumerator.h"
#include "../logger.h"
#include <QTimer>
#include <QTime>
#include <QByteArray>
#include <qmath.h>

namespace {
    const QByteArray CHECK_ADC = "\x03";
    const QByteArray CHECK_GPS = "\x04";
    const QByteArray START_RECEIVE_50 = "\x01";
    const QByteArray START_RECEIVE_200 = "\x02";
    const QByteArray STOP_RECEIVE("\x00", 1); // simply = "\x00" won't work: will be empty string
    const QByteArray CHECKED_ADC = CHECK_ADC;
    const QByteArray DATA_PREFIX(5, '\xF0');
    // GPS packets:
    const QByteArray GPS_PREFIX_TIME = "\x10\x41";
    const QByteArray GPS_PREFIX_POS  = "\x10\x4A";
    // GPS packets sizes:
    const int GPS_PSIZE_TIME = 10;
    const int GPS_PSIZE_POS  = 20;
    // GPS constants
    const QDateTime GPS_BASE_TIME(QDate(1980, 1, 6), QTime(0, 0), Qt::UTC);

    const int MIN_FREQUENCY = 1;
    const int POINTS_IN_PACKET = 200;
    const int DEFAULT_FILTER_FREQ = 200;

    /**
     * @brief Unpacks unsigned int of arbitrary length
     *        and _shifts_ rawData pointer by the size of unpacked data
     *
     * Usage: unpackUINT<unsigned short>(data) or unpackUINT<quint32>(data) etc
     * @param rawData - pointer to raw data in network byte order
     * @return resulting number
     */
    template <typename T>
    T unpackUINT(const char *& packet) {
        const quint8 * data = reinterpret_cast<const quint8*>(packet);
        T res = 0;
        // Use shift so that this code is independent on current arch
        // TODO: some more efficient algorithm.
        for (unsigned i = 0; i < sizeof(res); ++i) {
            res <<= 8;
            res |= data[i];
        }
        packet += sizeof(res);
        return res;
    }
    /**
     * @brief Unpacks 32-bit float
     *        and _shifts_ rawData pointer by the size of unpacked data
     *
     * @param rawData - pointer to raw data in network byte order
     * @return resulting number
     */
    float unpackFloat(const char *& packet) {
        // Collect binary representation as one 32-bit number
        // and  cast these bytes to float via union punning
        union { quint32 ui; float flt; } u;
        u.ui = unpackUINT<quint32>(packet);
        return u.flt;
    }
    // TODO: remove on update to Qt >= 5.1
    float radiansToDegrees(float radians) {
        return radians / M_PI * 180.0f;
    }
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

// TODO: split this function
void SerialProtocol::onDataReceived() {
    QByteArray rawData = port->readAll();

    if (debugMode) {
        Logger::trace(portName + ": " + rawData.toHex());
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
        } else if(rawData.startsWith(GPS_PREFIX_TIME)) {
            // TODO: support more GPS packets
            int offset = GPS_PREFIX_TIME.size();
            if (rawData.size() > GPS_PSIZE_TIME + offset) {
                // TODO: support accumulation
                // enough bytes, parse the packet
                const char * packet = rawData.constData() + offset;
                // note that each unpack* function moves `packet` pointer forward
                float timeOfWeek   = unpackFloat(packet);
                quint16 weekNumber = unpackUINT<quint16>(packet);
                float offsetUTC    = unpackFloat(packet);
                // TODO: millisecond precision... or not needed?
                QDateTime dateTime = GPS_BASE_TIME.addDays(7*weekNumber).addSecs(timeOfWeek - offsetUTC);
                Logger::info(tr("Received time update: [%1;%2;%3]=%4UTC")
                             .arg(timeOfWeek)
                             .arg(weekNumber)
                             .arg(offsetUTC)
                             .arg(dateTime.toString("yyyy-MM-dd hh:mm")));
                addState(GPSReady);
                // TODO: pass new time and set it as system time
                emit checkedGPS(true);
            } else {
                // TODO: accumulate bytes in buffer (not yet implemented)
            }
        } else if (rawData.startsWith(GPS_PREFIX_POS)) {
            // TODO: avoid copypaste from above! all issues inherited
            int offset = GPS_PREFIX_POS.size();
            if (rawData.size() > GPS_PSIZE_POS + offset) {
                const char* packet = rawData.constData() + offset;
                // note that each unpack* function moves `packet` pointer forward
                float latitude  = radiansToDegrees( unpackFloat(packet) );
                float longitude = radiansToDegrees( unpackFloat(packet) );
                Logger::info(tr("Received position update: %1, %2").arg(latitude).arg(longitude));
            } else {
                // TODO: accumulate bytes in buffer (not yet implemented)
            }
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
