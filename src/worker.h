#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include "protocol.h"

/*!
 * \brief The Worker class for controlling data processing process (pun intended)
 *
 * Normal workflow is as follows:
 *
 * - new Worker(protocol1, protocol2, this)
 * - Worker::prepare
 * - Worker::start
 * - get data via Worker::dataUpdated signal
 * - Worker::finish (called on destruction anyway)
 *
 * \note
 * Note that Worker::prepare may take time, that's why its result is
 * returned asynchronously via Worker::prepareFinished signal. You should
 * call Worker::start from a slot connected to this signal.
 */
class Worker : public QObject
{
    Q_OBJECT
public:
    /*!
     * \brief Result of Worker::prepare (returned asynchronously)
     */
    enum PrepareResult {
        PrepareSuccess,  /*! Worker is ready to be started and receive data */
        PrepareFailADC,  /*! Preparation failed because of ADC and Worker::prepare should be called again after fixing errors. */
        PrepareFailGPS,  /*! Preparation failed because of GPS and Worker::prepare should be called again after fixing errors. */
        PrepareAlready   /*! Preparation not needed: already prepared */
    };

    /*!
     * \brief Result of Worker::start
     */
    enum StartResult {
        StartSuccess,           /*! Started receiving data successfully */
        StartFailNotPrepared,   /*! Cannot start: Worker::prepare not called or failed */
        StartFailAlreadyStarted,/*! Cannot start: already started */
        StartFailDisconnected   /*! Cannot start: connection problems */
    };

    /*!
     * \brief Worker constructor does nothing. After creating, the worker should be moved
     *        to background thread, and then communicated via signals/slots
     * \param parent  - usual QObject parent parameter
     */
    explicit Worker(QObject *parent = nullptr);

    bool isPrepared() { return prepared; }

    /*!
     * \brief Check if Worker is started
     * \return true if Worker successfully started
     *
     * Consider using this getter to check if Worker is already started
     * before invoking Worker::start since calling it twice is illegal
     * \see Worker::start
     */
    bool isStarted() { return started; }

    ~Worker() { finish(); }

public slots:
    /*!
     * \brief Reset worker to given protocols
     *
     * All operation with previous protocol is interrupted.
     * Previous protocol is closed and disconnected.
     * After that Worker::prepare can be called again.
     * \param protADC - creator for protocol to be used for ADC from now
     * \param protGPS - creator for protocol to be used for GPS from now
     * \warning Worker takes ownership on protocol in order to prevent it from
     *          being deleted before Worker (and cause crash in destructor)
     */
    void reset(ProtocolCreator * protADC, ProtocolCreator * protGPS);

    /*!
     * \brief Prepare for data receiving
     *
     * Opens protocol, checks ADC and GPS and if all is OK and emits
     * Worker::prepareFinished when finished, with argument specifying whether it was successful
     * \param autostart if true, automatically start if prepared succesfully
     * \see Protocol::checkADC, Protocol::checkGPS
     */
    void prepare(bool autostart = false);

    /*!
     * \brief Start processing data.
     *
     * Will do it until Worker::finish is called
     */
    void start();

    /*!
     * \brief Stops processing data, but doesn't close protocol
     *
     * After then Worker can be started without calling Worker::prepare;
     */
    void stop();

    /*!
     * \brief Finish processing data and close protocol
     *
     * In order to start worker again, you should call
     * Worker::prepare again before calling Worker::start
     */
    void finish();

    /// The following actions are just forwarded to Protocol
    void setFrequencies(int samplingFreq, int filterFreq);

signals:
    /*!
     * \brief emitted when preparation process finished, either succesfully or not
     *
     * \param result preparation result, \see Worker::PrepareResult
     */
    void prepareFinished(PrepareResult result);

    /*!
     * \brief emitted after calling \a start to notify about whether it was successful
     *
     * \param result - result of starting, \see Worker::StartResult
     */
    void triedToStart(StartResult result);

    /*!
     * \brief emitted after \a stop was called and receiving stopped
     */
    void stopped();

    /*!
     * \brief emitted after the Worker was either started or stopped
     * \param started - the new state of Worker
     */
    void startedOrStopped(bool started);

    /*!
     * \brief emitted after \a finish() was called and protocols were closed
     */
    void finished();

    /// The following signals are just transmissions of Protocol ones

    /*!
     * \brief emitted when new data has come
     * \param newData - newly received data
     * \param newTimeStamps - timestamps for \a data
     * \see Worker::data
     */
    void dataUpdated(TimeStampsVector newTimeStamps, DataVector newData);
    /*! \see Protocol::checkedADC */
    void checkedADC(bool success);
    /*! \see Protocol::checkedGPS */
    void checkedGPS(bool success);
    /*! \see Protocol::timeAvailable */
    void timeAvailable(QDateTime timeGPS);
    /*! \see Protocol::positionAvailable */
    void positionAvailable(double latitiude, double longitude, double altitude);

private slots:
    void onCheckedADC(bool);
    void onCheckedGPS(bool);
private:
    void setPrepared(PrepareResult res);

    void assignProtocol(Protocol *& lvalue, ProtocolCreator * rvalue);
    void finalizeProtocol(Protocol * prot);

    Protocol * protocolADC_;
    Protocol * protocolGPS_;

    bool autostart;
    bool prepared;
    bool started;

};

#endif // WORKER_H
