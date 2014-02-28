#ifndef FILEWRITER_H
#define FILEWRITER_H

#include "protocol.h"

#include <QObject>
#include <QQueue>

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
    explicit FileWriter(QString fileName, QObject *parent = 0);

    QString fileName() { return saveFileName; }
    bool autoWriteEnabled() { return autoWrite; }

    ~FileWriter();
signals:
    void queueSizeChanged(unsigned newSize);
    
public slots:
    /*!
     * \brief Sets current file name that will be used for writing.
     *
     * \warning This slot only sets file name, it doesn't
     *          actually write to file!
     * \see Worker::setAutoWriteEnabled, Worker::writeOnce
     * \param fileName name of file to write to
     *
     * \todo return success or failure
     */
    void setFileName(QString fileName);

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
private:

    void writeNow();

    void openIfClosed();
    void closeIfOpened();

    QString saveFileName;
    QFile * file;
    bool autoWrite;

    QQueue<QString> waitingQueue;
};

#endif // FILEWRITER_H
