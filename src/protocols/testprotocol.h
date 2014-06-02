#ifndef TESTPROTOCOL_H
#define TESTPROTOCOL_H

#include "../protocol.h"
#include "../performancereporter.h"

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
    explicit TestProtocol(int dataSize = 100, int amp = 100,  QObject *parent = nullptr);
    QString description();

    bool open() override;
    void checkADC() override;
    void checkGPS() override;
    void startReceiving() override;
    void stopReceiving() override;
    void close() override;

    // Conform to Protocol frequency setting API (while not completely implemented)
    int  samplingFrequency() override;
    void setSamplingFrequency(int value) override;
    int  filterFrequency() override; // has no meaning, always returns 0
    void setFilterFrequency(int value) override; // has no meaning, does nothing

    ~TestProtocol();

    static PerformanceReporter perfReporter; // Bad to be global variable :( but for easier development usage...

private slots:

private:
    DataVector generateRandom(TimeStampsVector t);

    int dataSize;
    int amp;
    QTimer * dataTimer;
    QTimer * checkADCTimer;
    QTimer * checkGPSTimer;
};

#endif // TESTPROTOCOL_H
