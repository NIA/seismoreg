#ifndef FILEWRITER_H
#define FILEWRITER_H

#include "protocol.h"
#include "performancereporter.h"

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
    explicit FileWriter(QString outputDirectory = DEFAULT_OUTPUT_DIR, QString fileNameFormat = DEFAULT_FILENAME_FORMAT, QObject *parent = nullptr);

    static PerformanceReporter perfReporter; // Bad to be global variable :( but for easier development usage...

    QString outputDirectory() { return outputDir; }

    /*!
     * \brief String that describes filename format.
     *
     * Possible fields:
     * - %Y : year
     * - %M : month
     * - %D : day
     * - %h : hours
     * - %m : minutes
     * - %s : seconds
     * - %f : filter frequency
     * - %r : sampling frequency (rate)
     * - %i : device id
     *
     * Default is %D%M%Y-%h%m%s-%f.w%i
     *
     * \see fileNameFormatHelp() to show same help text in application
     * \return current filename format
     */
    QString fileNameFormat() const { return fileFormat; }
    static const QString DEFAULT_OUTPUT_DIR;
    static const QString DEFAULT_FILENAME_FORMAT;

    static QString fileNameFormatHelp();

    bool autoWriteEnabled() { return autoWrite; }

    QString buildFileName() const;

    ~FileWriter();
signals:
    void queueSizeChanged(unsigned newSize);
    
public slots:
    /*!
     * \brief Sets file name pattern and output dir for file that will be used for writing.
     *        Closes previously opened file, so that a new one with given outputDirectory
     *        and fileNameFormat will be opened on demand.
     *
     * \warning This slot only sets file name, it doesn't
     *          actually write to file or even open it!
     * \see Worker::setAutoWriteEnabled, Worker::writeOnce
     * \param outputDirectory - path to the directory where files will be created
     * \param fileNameFormat - pattern of filename, \see fileNameFormat() for info about format fields
     *
     * \todo return success or failure
     */
    void setFileName(QString outputDirectory, QString fileNameFormat);

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
    // TODO: (?) should we check that data are already being received,
    //           so the header is already written?

    void setDeviceID(int id) { deviceID = id; }
    void setCoordinates(double latitude, double longitude) {
        this->latitude  = QString::number(latitude, 'f', 6);
        this->longitude = QString::number(longitude, 'f', 6);
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

    QString outputDir;
    QString fileFormat;
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
