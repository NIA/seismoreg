#ifndef SERIALPROTOCOL_H
#define SERIALPROTOCOL_H

#include "../protocol.h"
#include "../performancereporter.h"
#include "qextserialport.h"
#include <QDateTime>

class QTimer;

struct PortSettingsEx : public PortSettings {
    // In addition to all its fields, one another:
    bool debug;
    // and constructor:
    PortSettingsEx(BaudRateType baudRate, DataBitsType dataBits, ParityType parity, StopBitsType stopBits, FlowType flowControl, long timeoutMillisec, bool debug);
    // and default constructor for convenience:
    PortSettingsEx() {}
};

class SerialProtocol : public Protocol
{
    Q_OBJECT
public:
    static const PortSettingsEx DEFAULT_PORT_SETTINGS;

    static PerformanceReporter perfReporter; // Bad to be global variable :( but for easier development usage...

    /*!
     * \brief SerialProtocol
     * \param portName - name (or path) of port to be opened. On Windows it is like COM1,
     *        while on *NIX it looks like path: i.e. /dev/ttyS0
     * \param samplingFrequency - number of points per second in result
     */
    explicit SerialProtocol(QString portName, int samplingFreq, int filterFreq, PortSettingsEx settings = DEFAULT_PORT_SETTINGS, QObject * parent = 0);
    QString description();

    bool open() override;
    void checkADC() override;
    void checkGPS() override;
    void startReceiving() override;
    void stopReceiving() override;
    void close() override;

    int  samplingFrequency() override;
    void setSamplingFrequency(int value) override;
    int  filterFrequency() override;
    void setFilterFrequency(int value) override;

    ~SerialProtocol();

    static QList<QString> portNames();

    /**
     * @brief Generates timestamps for received data
     *
     * If \a t0 is current time, it will create \a count timestamps
     * in the interval [t0 - periodMsecs, t0), with evenly distributed intervals
     * @param periodMsecs - size of interval
     * @param count - number of timestamps to be generated
     * @return the vector of timestamps
     */
    static TimeStampsVector generateTimeStamps(double periodMsecs, int count);

    enum GPSPacketType {
        GPSNoPacket, /*!< Currently not inside known packet (no data yet or unknown packet) */
        GPSTime,     /*!< Currently inside GPS Time packet (0x41)  */
        GPSHealth,   /*!< Currently inside GPS Health packet (0x46) */
        GPSPosition  /*!< Currently inside GPS Position packet (0x4A)  */
    };

private slots:
    void onDataReceived();

private:
    /**
     * @brief Find, detect, take and parse *one* GPS packet from \a buffer
     *
     * Should be invoked subsequently until returns false
     * @return true if found and parsed a packet, false otherwise
     */
    bool takeGPSPacket();

    void parseGPSPacket();

    QString portName;
    QextSerialPort * port;
    int samplingFrequency_;
    int filterFrequency_;
    QByteArray buffer;

    bool debugMode;

    // GPS packet parser state:
    GPSPacketType currentPacketGPS;
};

#endif // SERIALPROTOCOL_H
