#ifndef SERIALPROTOCOL_H
#define SERIALPROTOCOL_H

#include "../protocol.h"
#include "qextserialport.h"

class QTimer;

class SerialProtocol : public Protocol
{
    Q_OBJECT
public:
    static const BaudRateType DEFAULT_BAUD_RATE;
    static const BaudRateType GPS_BAUD_RATE;

    /*!
     * \brief SerialProtocol
     * \param portName - name (or path) of port to be opened. On Windows it is like COM1,
     *        while on *NIX it looks like path: i.e. /dev/ttyS0
     * \param samplingFrequency - number of points per second in result
     */
    explicit SerialProtocol(QString portName, int samplingFrequency, BaudRateType baudRate = DEFAULT_BAUD_RATE, bool debug = false, QObject * parent = 0);
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
    int frequency;
    QByteArray buffer;

    bool debugMode;
};

#endif // SERIALPROTOCOL_H
