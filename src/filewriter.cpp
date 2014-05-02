#include "filewriter.h"

#include "logger.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>


const QString FileWriter::DEFAULT_FILENAME_PREFIX = "data-";
const QString FileWriter::DEFAULT_FILENAME_SUFFIX = ".dat";

FileWriter::FileWriter(QString fileNamePrefix, QString fileNameSuffix, QObject *parent) :
    QObject(parent), file(NULL), autoWrite(false),
    deviceID(0), samplingFreq(0), filterFreq(0),
    latitude("???"), longitude("???"), itemsInQueue(0)
{
    setFileName(fileNamePrefix, fileNameSuffix); // will also init `file` instance variable
}

QString FileWriter::buildFileName() {
    const TimeStampType &time = (startTime.isValid() ? startTime : TimeStampType::currentDateTime());
    return filePrefix + time.toString("yyyy-MM-dd-hh-mm-ss") + fileSuffix;
}

void FileWriter::setFileName(QString prefix, QString suffix) {
    if (suffix.isEmpty()) {
        suffix = DEFAULT_FILENAME_SUFFIX;
    }
    if ((prefix == filePrefix) && (suffix == fileSuffix)) {
        return; // Nothing changed
    }
    filePrefix = prefix;
    fileSuffix = suffix;
    closeIfOpened();
}

void FileWriter::receiveData(TimeStampsVector t, DataVector d) {
    int count = qMin(t.size(), d.size()); // TODO: warn if different or empty
    if (count == 0) { return; }

    if (startTime.isNull()) { // Not set yet
        startTime = t.first();
    }

    QString allData;
    char buffer[100];
    foreach(const DataItem & item, d) {
        for(unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
            // TODO: using non-standard _itoa_s! but it is much faster than QString::number
            _itoa_s(item.byChannel[ch], buffer, sizeof(buffer), 10);
            allData += buffer;
            allData += '\t';
        }
        allData += '\n';
    }
    waitingQueue.enqueue(allData);
    itemsInQueue += d.size()*CHANNELS_NUM;

    emit queueSizeChanged(itemsInQueue);

    if (autoWrite) {
        writeNow();
    }
}

void FileWriter::setAutoWriteEnabled(bool enabled) {
    if (autoWrite == enabled) {
        return; // Nothing changed
    }

    autoWrite = enabled;
    if (enabled) {
        // TODO: check if can write and disable autoWrite if cannot
        if (! waitingQueue.isEmpty()) {
            writeNow();
        }
    } else {
        closeIfOpened();
    }
}

void FileWriter::writeOnce() {
    writeNow();
    closeIfOpened();
}

void FileWriter::writeNow() {
    if(waitingQueue.isEmpty()) {
        Logger::warning(tr("Nothing to write to file"));
        return; // Nothing to write
    }

    if ( openIfClosed() == false ) { return; } // Failed to open

    // Swap existing queue with empty one to quickly move all from waiting queue to writeQueue and clear writeQueue
    QQueue<QString> writeQueue;
    writeQueue.swap(waitingQueue);
    int itemsWritten = itemsInQueue;
    itemsInQueue = 0;
    emit queueSizeChanged(itemsInQueue);

    // And write everything from writeQueue
    QTextStream out(file);
    foreach (auto str, writeQueue) {
        out << str;
    }
    Logger::trace(tr("Written %1 items to file %2").arg(itemsWritten).arg(file->fileName()));
}

bool FileWriter::openIfClosed() {
    if (file == NULL) {
        file = new QFile(buildFileName());
    }
    if ( ! file->isOpen()) {
        bool newFile = ! file->exists();
        bool success = file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        if ( ! success ) {
            Logger::error(tr("Failed to open file %1: %2").arg(file->fileName()).arg(file->errorString()));
            return false;
        } else {
            Logger::info(tr("Opened file %1").arg(file->fileName()));
            if (newFile) {
                writeHeader();
            }
            return true;
        }
    }
    return true;
}

void FileWriter::writeHeader() {
    QTextStream out(file);
    out << QStringLiteral("[Description]\nDevice ID=%1\n").arg(deviceID);
    out << QStringLiteral("[Frequency]\n%1 Hz\n").arg(samplingFreq);
    out << QStringLiteral("[Time]\n%1\n").arg(startTime.toString("hh:mm:ss"));
    out << QStringLiteral("[Coordinates]\n%1\n%2\n").arg(latitude).arg(longitude);
    out << QStringLiteral("[Date]\n%1\n").arg(startTime.toString("yyyy-MM-dd"));
    out << QStringLiteral("[Values]\n");
}

void FileWriter::closeIfOpened() {
    waitingQueue.clear();
    itemsInQueue = 0;
    emit queueSizeChanged(itemsInQueue);
    startTime = TimeStampType(); // set null datetime so that it will be reset next time
    if(file != NULL && file->isOpen()) {
        file->close();
        Logger::info(tr("Closed file %1").arg(file->fileName()));
    }
    delete file;
    file = NULL;
}

FileWriter::~FileWriter() {
    closeIfOpened();
}
