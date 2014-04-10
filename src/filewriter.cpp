#include "filewriter.h"

#include "logger.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

FileWriter::FileWriter(QString fileName, QObject *parent) :
    QObject(parent), file(NULL), autoWrite(false),
    deviceID(0), samplingFreq(0), filterFreq(0),
    latitude("???"), longitude("???")
{
    setFileName(fileName); // will also init `file` instance variable
}

QString FileWriter::defaultFileName() {
    return QString("data-%1.dat").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss"));
}

void FileWriter::setFileName(QString fileName) {
    if (fileName == saveFileName) {
        return; // Nothing changed
    }
    saveFileName = fileName;
    closeIfOpened();
    delete file;
    file = new QFile(saveFileName);
}

void FileWriter::receiveData(TimeStampsVector t, DataVector d) {
    int count = qMin(t.size(), d.size()); // TODO: warn if different or empty
    if (count == 0) { return; }

    if (startTime.isNull()) { // Not set yet
        startTime = t.first();
    }

    QString allData;
    foreach(const DataItem & item, d) {
        for(unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
            allData += QString::number(item.byChannel[ch]);
            allData += '\t';
        }
        allData += '\n';
    }
    waitingQueue.enqueue(allData);

    emit queueSizeChanged(waitingQueue.size());

    if (autoWrite) {
        writeNow();
    }
}

void FileWriter::setAutoWriteEnabled(bool enabled) {
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
    if (file == NULL) { return; } // TODO: report internal error

    if(waitingQueue.isEmpty()) {
        Logger::warning(tr("Nothing to write to file"));
        return; // Nothing to write
    }

    openIfClosed();

    // Swap existing queue with empty one to move all from waiting queue to writeQueue
    QQueue<QString> writeQueue;
    writeQueue.swap(waitingQueue);
    emit queueSizeChanged(waitingQueue.size());

    // And write everything from writeQueue
    QTextStream out(file);
    int itemsWritten = writeQueue.size();
    foreach (auto str, writeQueue) {
        out << str;
    }
    Logger::trace(tr("Written %1 items to file %2").arg(itemsWritten).arg(file->fileName()));
}

void FileWriter::openIfClosed() {
    if (file == NULL) { return; } // TODO: report internal error

    if ( ! file->isOpen()) {
        bool newFile = ! file->exists();
        bool success = file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        if ( ! success ) {
            Logger::error(tr("Failed to open file %1: %2").arg(file->fileName()).arg(file->errorString()));
            return; // TODO: report error
        } else {
            Logger::info(tr("Opened file %1").arg(file->fileName()));
            if (newFile) {
                writeHeader();
            }
        }
    }
}

void FileWriter::writeHeader() {
    QTextStream out(file);
    out << QString("[Description]\nDevice ID=%1\n").arg(deviceID);
    out << QString("[Frequency]\nSamples=%1 Hz\nFilter=%2 Hz\n").arg(samplingFreq).arg(filterFreq);
    out << QString("[Date]\n%1\n").arg(startTime.toString("yyyy-MM-dd"));
    out << QString("[Time]\n%1\n").arg(startTime.toString("hh:mm:ss.zzz"));
    out << QString("[Coordinates]\nLatitude=%1\nLongitude=%2\n").arg(latitude).arg(longitude);
    out << QString("[Values]\n");
}

void FileWriter::closeIfOpened() {
    if(file != NULL && file->isOpen()) {
        file->close();
        Logger::info(tr("Closed file %1").arg(file->fileName()));
    }
}

FileWriter::~FileWriter() {
    closeIfOpened();
}
