#ifndef TESTPROTOCOL_H
#define TESTPROTOCOL_H

#include "../protocol.h"

class QTimer;

class TestProtocol : public Protocol
{
    Q_OBJECT
public:
    /*!
     * \brief TestProtocol
     * \param dataSize number of datapoints that will be returned at once
     * \param mean the mean value of random data that will be generated for testing
     * \param parent usual QObject parent argument
     */
    explicit TestProtocol(int dataSize = 100, int mean = 100, QObject *parent = 0);
    QString description();

    virtual bool open();
    virtual void checkADC();
    virtual void checkGPS();
    virtual void startReceiving();
    virtual void stopReceiving();
    virtual void close();

    ~TestProtocol();

private slots:

private:
    DataVector generateRandom();

    int dataSize;
    int mean;
    QTimer * dataTimer;
    QTimer * checkADCTimer;
    QTimer * checkGPSTimer;
};

#endif // TESTPROTOCOL_H
