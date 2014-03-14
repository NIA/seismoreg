#ifndef SERIALPROTOCOL_H
#define SERIALPROTOCOL_H

#include "../protocol.h"

class QTimer;
class QextSerialPort;

class SerialProtocol : public Protocol
{
    Q_OBJECT
public:
    /*!
     * \brief SerialProtocol
     * \param portName - name (or path) of port to be opened. On Windows it is like COM1,
     *        while on *NIX it looks like path: i.e. /dev/ttyS0
     * \param packetSize - number of points in packet
     */
    explicit SerialProtocol(QString portName, int samplingFrequency, QObject * parent = 0);
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
};

#endif // SERIALPROTOCOL_H
