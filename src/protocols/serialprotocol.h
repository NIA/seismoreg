#ifndef SERIALPROTOCOL_H
#define SERIALPROTOCOL_H

#include "../protocol.h"
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

    /*!
     * \brief SerialProtocol
     * \param portName - name (or path) of port to be opened. On Windows it is like COM1,
     *        while on *NIX it looks like path: i.e. /dev/ttyS0
     * \param samplingFrequency - number of points per second in result
     */
    explicit SerialProtocol(QString portName, int samplingFreq, int filterFreq, PortSettingsEx settings = DEFAULT_PORT_SETTINGS, QObject * parent = 0);
    QString description();

    virtual bool open();
    virtual void checkADC();
    virtual void checkGPS();
    virtual void startReceiving();
    virtual void stopReceiving();
    virtual void close();

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

private slots:
    void onDataReceived();

private:
    DataVector generateRandom();

    QString portName;
    QextSerialPort * port;
    int samplingFrequency;
    int filterFrequency;
    QByteArray buffer;

    bool debugMode;
};

#endif // SERIALPROTOCOL_H
