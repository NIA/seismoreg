#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include "protocol.h"

/*!
 * \brief The Worker class for controlling data processing process (pun intended)
 *
 * Normal workflow is as follows:
 *
 * - new Worker(protocol, this)
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
     * \param prot protocol to be used
     * \param parent usual QObject parent parameter
     * \warning Worker takes ownership on protocol in order to prevent it from
     *          being deleted before Worker (and cause crash in destructor)
     */
    explicit Worker(Protocol * prot, QObject *parent = 0);

    /*!
     * \brief Reset worker to given protocol
     *
     * All operation with previous protocol is interrupted.
     * Data is cleared.
     * After that Worker::prepare can be called again.
     * \param prot protocol to be used from now
     * \warning Worker takes ownership on protocol in order to prevent it from
     *          being deleted before Worker (and cause crash in destructor)
     */
    void reset(Protocol * prot);

    /*!
     * \brief Getter for connecting to signals of Protocol
     * \return current protocol in action
     */
    Protocol * protocol() { return protocol_; }

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
     * \brief Getter for accessing entire data vector
     *
     * If you need only last added data items instead, they are passed to Worker::dataUpdated signal
     * \return all data items received from the begining (or from the last Worker::reset)
     * \see Worker::dataUpdated
     */
    DataVector data() { return data_; }

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
     * \param newData newly received data items
     * \see Worker::data
     */
    void dataUpdated(DataVector newData);

private slots:
    void onCheckedADC(bool);
    void onCheckedGPS(bool);
    void onDataAvailable(DataVector);
private:
    void setPrepared(PrepareResult res);

    Protocol * protocol_;
    DataVector data_;

    bool autostart;
    bool prepared;
    bool started;
    bool paused;

};

#endif // WORKER_H
