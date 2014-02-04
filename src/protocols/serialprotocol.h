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
    explicit SerialProtocol(QString portName, int pointsPerChannel, QObject * parent = 0);
    QString description();

    virtual bool open();
    virtual void checkADC();
    virtual void checkGPS();
    virtual void startReceiving();
    virtual void stopReceiving();
    virtual void close();

    static QList<QString> portNames();

private slots:
    void onDataReceived();

private:
    DataVector generateRandom();

    QString portName;
    QextSerialPort * port;
    int pointsInPacket;
    QByteArray buffer;
};

#endif // SERIALPROTOCOL_H
