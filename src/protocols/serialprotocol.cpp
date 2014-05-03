#include "serialprotocol.h"
#include "qextserialenumerator.h"
#include "../logger.h"
#include <QTimer>
#include <QTime>
#include <QByteArray>
#include <QDateTime>
#include <qmath.h>

namespace {
    const QByteArray CHECK_ADC = "\x03";
    const QByteArray START_RECEIVE_50 = "\x01";
    const QByteArray START_RECEIVE_200 = "\x02";
    const QByteArray STOP_RECEIVE("\x00", 1); // simply = "\x00" won't work: will be empty string
    const QByteArray CHECKED_ADC = CHECK_ADC;
    const QByteArray DATA_PREFIX(5, '\xF0');
    // GPS commands:
    const QByteArray GPS_REQUEST_TIME = "\x10\x21\x10\x03";
    // GPS packets:
    struct KnownPacket {
        SerialProtocol::GPSPacketType kind;
        QByteArray prefix;
        int size;
    };
    const KnownPacket KNOWN_GPS_PACKETS[] = {
        //               kind         preifx      size
        {SerialProtocol::GPSHealth,   "\x10\x46", 2},
        {SerialProtocol::GPSTime,     "\x10\x41", 10},
        {SerialProtocol::GPSPosition, "\x10\x4A", 20},
    };
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
    double radiansToDegrees(double radians) {
        return radians / M_PI * 180.0;
    }
}

PortSettingsEx::PortSettingsEx(BaudRateType baudRate, DataBitsType dataBits, ParityType parity, StopBitsType stopBits, FlowType flowControl, long timeoutMillisec, bool debug)
    : PortSettings({baudRate, dataBits, parity, stopBits, flowControl, timeoutMillisec}),
      debug(debug)
{}


const PortSettingsEx SerialProtocol::DEFAULT_PORT_SETTINGS(BAUD115200, DATA_8, PAR_NONE, STOP_1, FLOW_OFF, 10, false);
PerformanceReporter  SerialProtocol::perfReporter("COM");

PerformanceReporter  SerialProtocol::generateTimestampsPerfReporter("generateTimestamps");

SerialProtocol::SerialProtocol(QString portName, int samplingFreq, int filterFreq, PortSettingsEx settings, QObject *parent) :
    Protocol(parent), portName(portName), port(NULL), samplingFrequency_(samplingFreq), filterFrequency_(filterFreq),
    debugMode(settings.debug), currentPacketGPS(GPSNoPacket)
{
    port = new QextSerialPort(portName);
    port->setBaudRate(settings.BaudRate);
    port->setDataBits(settings.DataBits);
    port->setStopBits(settings.StopBits);
    port->setParity(settings.Parity);
    port->setFlowControl(settings.FlowControl);
    // TODO: support timeout setting?

    if (samplingFrequency_ < MIN_FREQUENCY) {
        Logger::error(tr("Incorrect frequency: cannot be less than %1").arg(MIN_FREQUENCY));
        samplingFrequency_ = MIN_FREQUENCY;
    }
    if (POINTS_IN_PACKET % samplingFrequency_ != 0) { // and also if frequency > POINTS_IN_PACKET
        Logger::error(tr("Incorrect frequency: should be a divisor of %1").arg(POINTS_IN_PACKET));
        samplingFrequency_ = POINTS_IN_PACKET;
    }
    perfReporter.setDescription(description());
}

QString SerialProtocol::description() {
    return tr("Serial port %1 [%2->%3]").arg(portName).arg(POINTS_IN_PACKET).arg(samplingFrequency_);
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
    addState(ADCWaiting);
    port->write(CHECK_ADC);
}

void SerialProtocol::checkGPS() {
    addState(GPSWaiting); // Note that this state will not be removed: will always wait for future GPS updates
    port->write(GPS_REQUEST_TIME);
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
    if (filterFrequency_ == DEFAULT_FILTER_FREQ) {
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

int SerialProtocol::samplingFrequency() {
    return samplingFrequency_;
}
void SerialProtocol::setSamplingFrequency(int value) {
    if (hasState(Receiving)) {
        Logger::error(tr("Cannot change parameters when receiving data"));
    }
    samplingFrequency_ = value;
}

int SerialProtocol::filterFrequency() {
    return filterFrequency_;
}
void SerialProtocol::setFilterFrequency(int value) {
    if (hasState(Receiving)) {
        Logger::error(tr("Cannot change parameters when receiving data"));
    }
    filterFrequency_ = value;
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
            perfReporter.start();
            // If we see packet start:
            // Drop buffer
            buffer.clear();
            // Remove prefix
            rawData.remove(0, DATA_PREFIX.size()); // TODO: optimize? (avoid removing from beginning here and below)
        } else {
            perfReporter.unpause();
        }
        // Add data to buffer
        buffer += rawData;
        // if there is enough data in buffer to form and unwrap a packet, make it
        const int packetSize = CHANNELS_NUM*POINTS_IN_PACKET*sizeof(DataType); // TODO: make either global const or field
        while(buffer.size() >= packetSize) { // TODO: this is actually not correct way to handle two subsequent packets (header not removed)
            // allocate space for data array
            DataVector packetData(samplingFrequency_);
            // Unwrap data:
            if (samplingFrequency_ == POINTS_IN_PACKET) {
                // Easy case: just copy
                memcpy(packetData.data(), buffer.constData(), packetSize);
            } else {
                // Hard case: compute average of each avgSize items into one point
                int avgSize = POINTS_IN_PACKET / samplingFrequency_; // frequency must not be and must not be greater that POINTS_IN_PACKET: it is checked in constructor
                const DataItem* curItem = reinterpret_cast<const DataItem*>(buffer.constData());
                for (int i = 0; i < samplingFrequency_; ++i) {
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
            perfReporter.stop();
            // notify
            emit dataAvailable(timeStamps, packetData);
        }
        perfReporter.pause();
    // If not receiving, then waiting either for ADC or for GPS
    } else if (hasState(ADCWaiting)) {
        if(rawData.startsWith(CHECKED_ADC)) {
            addState(ADCReady);
            removeState(ADCWaiting);
            emit checkedADC(true);
        }
    } else /* if (hasState(GPSWaiting)) */ {
        buffer += rawData;
        bool isParsed;
        do {
            isParsed = takeGPSPacket();
        } while (isParsed);
    }
}

bool SerialProtocol::takeGPSPacket() {
    if (currentPacketGPS == GPSNoPacket) {
        // Try to detect packet
        int startPos = -1;
        KnownPacket detectedPacketInfo;
        for (const KnownPacket &packetInfo: KNOWN_GPS_PACKETS) {
            int newStartPos = buffer.indexOf(packetInfo.prefix);
            if (newStartPos >= 0) {
                // If found, then check if it is before previously detected:
                // (or if there is no previously detected: startPos < 0)
                if ((newStartPos < startPos) || (startPos < 0)) {
                    detectedPacketInfo = packetInfo;
                    startPos = newStartPos;
                }
            }
        }
        // If successfully detected
        if (startPos >= 0) {
            // Cut everything before packet (and header as well)
            currentPacketGPS = detectedPacketInfo.kind;
            buffer = buffer.mid(startPos + detectedPacketInfo.prefix.size());
        }
    }
    if (currentPacketGPS != GPSNoPacket) {
        // Try to parse packet. First, find which is the current packet
        // TODO: use QMap?
        for (const KnownPacket &packetInfo: KNOWN_GPS_PACKETS) {
            if(packetInfo.kind == currentPacketGPS) {
                if (buffer.size() > packetInfo.size) {
                    // enough bytes, parse the packet
                    parseGPSPacket();
                    // remove packet from buffer
                    buffer.remove(0, packetInfo.size);
                    return true;
                }
                // Not enough, wait for the next sending
                break;
            }
        }
    } else {
        // All that's left is parts of unsupported packets: clear the buffer
        buffer.clear();
        // NB: we may still lose some meaningful packet if its header gets
        // split between sendings, but this is quite unlikely
    }
    return false;
}

void SerialProtocol::parseGPSPacket() {
    // NB: assume we have enough bytes in buffer to parse packet: check this before!
    const char * packet = buffer.constData();
    // note that each unpack* function moves `packet` pointer forward
    switch (currentPacketGPS) {
    case GPSTime: {
        // Check if we previously received 0x46 Health packet with success code
        if (hasState(GPSReady)) {
            float timeOfWeek   = unpackFloat(packet);
            quint16 weekNumber = unpackUINT<quint16>(packet);
            float offsetUTC    = unpackFloat(packet);
            QDateTime dateTime = GPS_BASE_TIME.addDays(7*weekNumber).addMSecs((timeOfWeek - offsetUTC)*1000);
            addState(GPSHasTime);
            emit timeAvailable(dateTime);
        }
        break;
    }
    case GPSPosition: {
        double latitude  = radiansToDegrees( unpackFloat(packet) );
        double longitude = radiansToDegrees( unpackFloat(packet) );
        double altitude  = unpackFloat(packet);
        addState(GPSHasPos);
        emit positionAvailable(latitude, longitude, altitude);
        break;
    }
    case GPSHealth: {
        // First byte 0x00 means OK
        if (packet[0] == 0) {
            addState(GPSReady);
        }
        break;
    }
    default:
        // This is actually an internal error
        Logger::warning(tr("GPS packet not yet supported"));
        break;
    }
    currentPacketGPS = GPSNoPacket;
    if (hasState(GPSHasPos) && hasState(GPSHasTime)) {
        emit checkedGPS(true);
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
    generateTimestampsPerfReporter.start();
    TimeStampsVector res(count);

    TimeStampType start = QDateTime::currentMSecsSinceEpoch() - periodMsecs;
    double deltaMsecs = periodMsecs / count;
    for (int i = 0; i < count; ++i) {
        res[i] = start + i*deltaMsecs;
    }
    generateTimestampsPerfReporter.stop();
    return res;
}
