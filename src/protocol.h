#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QObject>
#include <QVector>

typedef int DataType;
const unsigned CHANNELS_NUM = 3;
struct DataItem {
    DataType byChannel[CHANNELS_NUM];
};
typedef QVector<DataItem> DataVector;

/*!
 * \interface Protocol
 * \brief The common Protocol interface (abstract class, to be precise - \see Protocol::state)
 *
 * Specifies higher-order abstraction for commands of
 * data-transferring protocol, *independent* on transport type
 * (socket, serial port etc).
 *
 * \remarks You would like to use some of the implementations of this
 * interface: SerialProtocol, TestProtocol or any other.
 */
class Protocol : public QObject
{
    Q_OBJECT
public:
    /*!
     * Possible states of protocol
     */
    enum SingleState {
        NoState   = 0,      /*!< Initial state: just created, nothing done */
        Open      = 1 << 0, /*!< Established connection, now can transmit and receive data */
        ADCReady  = 1 << 1, /*!< Validated ADC */
        GPSReady  = 1 << 2, /*!< Validated GPS */
        Receiving = 1 << 3  /*!< Listening for incoming data. Possible when: (1) connected, (2) validated ADC, (3) validated GPS */
    };
    Q_DECLARE_FLAGS(State, SingleState)

    /*!
     * \brief Standart QObject-like constructor for Protocol
     *
     * \warning When subclassing, don't forget to call this constructor
     *          from your initializer list and pass the parent, in order
     *          to let QObject's memory management work properly
     * \param parent QObject parent
     */
    explicit Protocol(QObject * parent = 0) : QObject(parent) { resetState(); }

    /*!
     * \brief Short description of protocol
     * \return translatable description of protocol type and its parameters (like port, etc)
     */
    virtual QString description() = 0;

    /*!
     * \brief initialize protocol communication
     * \todo probably this method should be async
     * \returns true if successful, false otherwise
     */
    virtual bool open() = 0;

    /*!
     * \brief sends ADC checking command
     *
     * Result is returned asynchronously via checkedADC signal
     * \see Protocol::checkedADC
     */
    virtual void checkADC() = 0;

    /*!
     * \brief sends GPS checking command
     *
     * Result is returned asynchronously via checkedGPS signal
     * \see Protocol::checkedGPS
     */
    virtual void checkGPS() = 0;

    /*!
     * \brief initiate data receiving
     *
     * Sends command to ADC so that it will begin sending data.
     * When new data is available, it is returned asynchronously via dataAvailable signal;
     * \see Protocol::dataAvailable
     */
    virtual void startReceiving() = 0;

    /*!
     * \brief initiate data receiving
     *
     * Stops processing received data.
     * In means simply ignoring all later data:
     */
    virtual void stopReceiving() = 0;

    /*!
     * \return current state
     */
    State state() const { return state_; }

    /*!
     * \brief checks if given state flag is set
     * \param flag flag to check
     * \return true if flag set, false otherwise
     */
    bool hasState(SingleState flag) const { return state_.testFlag(flag); }

    /*!
     * \brief Close all opened ports and finalize all work with protocol
     */
    virtual void close() = 0;

signals:
    /*!
     * \brief emitted when ADC check result is ready
     * \param success is true if ADC reported to be ready, false otherwise
     * \see Protocol::checkADC
     */
    void checkedADC(bool success);

    /*!
     * \brief emitted when GPS check result is ready
     * \param success is true if GPS reported to be ready, false otherwise
     * \see Protocol::checkGPS
     */
    void checkedGPS(bool success);

    /*!
     * \brief emitted when new data from ADC is available
     * \param data newly received data
     * \see Protocol::startReceiving, Protocol::stopReceiving
     */
    void dataAvailable(DataVector data);

    /*!
     * \brief emitted when state is changed
     * \param newState state value after change
     * \see Protocol::state
     */
    void stateChanged(State newState);

protected:
    void resetState()                      { setState(NoState); }
    void addState(State newState)          { setState(state_ | newState); }
    void removeState(SingleState oldState) { setState(state_ & (~oldState)); }
    void setState(State newState) {
        State oldState = state_;
        state_ = newState;
        if(oldState != newState) {
            emit stateChanged(newState);
        }
    }
private:
    State state_;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Protocol::State)

#endif // PROTOCOL_H
