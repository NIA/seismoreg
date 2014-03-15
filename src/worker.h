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
        PrepareFail,     /*! Preparation failed and Worker::prepare should be called again after fixing errors. */
        PrepareAlready   /*! Preparation not needed: already prepared */
        // TODO: PrepareFail expanded variants
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
     * \brief Worker constructor. Initializes worker with given protocol
     * \param protADC - protocol to be used for communication with ADC (send commands, receive data)
     * \param protGPS - protocol to be used for communication with GPS (receive time and coordinates).
     *                  Can be the same as protADC or not.
     * \param parent  - usual QObject parent parameter
     * \warning Worker takes ownership on protocols in order to prevent it from
     *          being deleted before Worker (and cause crash in destructor)
     */
    explicit Worker(Protocol * protADC, Protocol * protGPS, QObject *parent = 0);

    /*!
     * \brief Reset worker to given protocols
     *
     * All operation with previous protocol is interrupted.
     * Previous protocol is closed and disconnected.
     * After that Worker::prepare can be called again.
     * \param protADC - protocol to be used for ADC from now
     * \param protGPS - protocol to be used for GPS from now
     * \warning Worker takes ownership on protocol in order to prevent it from
     *          being deleted before Worker (and cause crash in destructor)
     */
    void reset(Protocol * protADC, Protocol * protGPS);

    /*!
     * \brief Getter for connecting to signals of Protocol
     * \return current protocol for ADC
     */
    Protocol * protocolADC() { return protocolADC_; }
    /*!
     * \brief Getter for connecting to signals of Protocol
     * \return current protocol for GPS
     */
    Protocol * protocolGPS() { return protocolGPS_; }

    /*!
     * \brief Prepare for data receiving
     *
     * Opens protocol, checks ADC and GPS and if all is OK and emits
     * Worker::prepareFinished when finished, with argument specifying whether it was successful
     * \param autostart if true, automatically start if prepared succesfully
     * \see Protocol::checkADC, Protocol::checkGPS
     */
    void prepare(bool autostart = false);

    bool isPrepared() { return prepared; }

    /*!
     * \brief Start processing data.
     *
     * Will do it until Worker::finish is called
     */
    StartResult start();

    /*!
     * \brief Check if Worker is started
     * \return true if Worker successfully started
     *
     * Consider using this getter to check if Worker is already started
     * before invoking Worker::start since calling it twice is illegal
     * \see Worker::start
     */
    bool isStarted() { return started; }

    /*!
     * \brief Pause receiving data without closing protocol
     */
    void pause();

    /*!
     * \brief Continue receiving data after pause
     */
    void unpause();

    bool isPaused() { return paused; }

    /*!
     * \brief Finish processing data and close protocol
     *
     * In order to start worker again, you should call
     * Worker::prepare again before calling Worker::start
     */
    void finish();

    ~Worker() { finish(); }

signals:
    /*!
     * \brief emitted when preparation process finished, either succesfully or not
     *
     * \param result preparation result, \see Worker::PrepareResult
     */
    void prepareFinished(PrepareResult result);

    /*!
     * \brief emitted when new data has come
     * \param newData - newly received data
     * \param newTimeStamps - timestamps for \a data
     * \see Worker::data
     */
    void dataUpdated(TimeStampsVector newTimeStamps, DataVector newData);

private slots:
    void onCheckedADC(bool);
    void onCheckedGPS(bool);
private:
    void setPrepared(PrepareResult res);

    void assignProtocol(Protocol *& lvalue, Protocol * rvalue);
    void finalizeProtocol(Protocol * prot);

    Protocol * protocolADC_;
    Protocol * protocolGPS_;

    bool autostart;
    bool prepared;
    bool started;
    bool paused;

};

#endif // WORKER_H
