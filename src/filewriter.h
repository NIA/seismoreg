#ifndef FILEWRITER_H
#define FILEWRITER_H

#include "protocol.h"

#include <QObject>
#include <QQueue>
#include <QDateTime>

class QFile;

/*!
 * \brief FileWriter maintains queue of data that should be written to file
 *        and writes them when requested.
 *
 * This class can be running in a separate thread, that's why
 * its slots should not be called directly (instead connect them to some signals)
 *
 * When the file will be actually opened:
 *   - If autowrite is disabled, the file will be opened when calling
 *     FileWriter::writeOnce and closed after that
 *   - If autowrite is enabled, the file will be opened when received first data
 *     via slot FileWriter::receiveData.
 *   - If autowrite is enabled AND file is already opened, setting a new file name
 *     with FileWriter::setFileName will cause the old file to be closed
 *     and the new one will be opened immediately.
 *
 * When the data will be written to file:
 *   - If autowrite is enabled, the data will be written to file
 *     as soon as they are received.
 *   - If autowrite is disabled, the data that is already in queue
 *     will be written when calling FileWriter::writeOnce
 */
class FileWriter : public QObject
{
    Q_OBJECT
public:
    explicit FileWriter(QString fileNamePrefix = DEFAULT_FILENAME_PREFIX, QString fileNameSuffix = DEFAULT_FILENAME_SUFFIX, QObject *parent = 0);

    QString fileNamePrefix() { return filePrefix; }
    QString fileNameSuffix() { return fileSuffix; }
    static const QString DEFAULT_FILENAME_PREFIX;
    static const QString DEFAULT_FILENAME_SUFFIX;

    bool autoWriteEnabled() { return autoWrite; }

    QString buildFileName();

    ~FileWriter();
signals:
    void queueSizeChanged(unsigned newSize);
    
public slots:
    /*!
     * \brief Sets file name pattern for file that will be used for writing.
     *
     * Filename will be prefix+datetime+suffix, where datetime is
     * the start time of receiving data.
     *
     * If suffix is empty (or not specified), the default (e.g. ".dat")
     * will be used.
     *
     * \warning This slot only sets file name, it doesn't
     *          actually write to file!
     * \see Worker::setAutoWriteEnabled, Worker::writeOnce
     * \param prefix - beginning of file name, before datetime
     * \param suffix - ending of file name, usually extension
     *
     * \todo return success or failure
     */
    void setFileName(QString prefix, QString suffix = DEFAULT_FILENAME_SUFFIX);

    /*!
     * \brief Adds new data to queue of data waiting to be written to disk
     * \param d - new data values
     */
    void receiveData(TimeStampsVector t, DataVector d);

    /*!
     * \brief Enables or disables auto-writing (data written as soon as received)
     *
     * The file will be closed after disabling auto write
     */
    void setAutoWriteEnabled(bool enabled);

    /*!
     * \brief Opens file, writes all data currently in queue, closes file
     */
    void writeOnce();

    /*!
     * \brief Finishes and closes file: newly received data will cause a new file to be opened.
     *
     * Useful after closing protocol before opening it with another settings,
     * or when the size of file exceeds some limit
     */
    void finishFile() { closeIfOpened(); }

    // Settings for header:

    void setDeviceID(int id) { deviceID = id; }
    void setCoordinates(QString latitude, QString longitude) {
        this->latitude  = latitude;
        this->longitude = longitude;
    }
    void setFrequencies(int samplingFreq, int filterFreq) {
        this->samplingFreq = samplingFreq;
        this->filterFreq   = filterFreq;
    }

private:

    void writeNow();
    void writeHeader();

    /**
     * @brief Opens file if it is not opened yet
     * @return true if succesfully opened (or already opened) and ready to write,
     *         false if failed to open and cannot write
     */
    bool openIfClosed();
    void closeIfOpened();

    QString filePrefix;
    QString fileSuffix;
    QFile * file;
    bool autoWrite;

    int deviceID;
    int samplingFreq;
    int filterFreq;
    QString latitude;
    QString longitude;
    QDateTime startTime;

    QQueue<QString> waitingQueue;
    int itemsInQueue; // Since multiple date items are in one waitingQueue item, a separate count is needed
};

#endif // FILEWRITER_H
