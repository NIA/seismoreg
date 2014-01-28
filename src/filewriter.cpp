#include "filewriter.h"

#include "logger.h"

#include <QFile>
#include <QTextStream>

FileWriter::FileWriter(QString fileName, QObject *parent) :
    QObject(parent), file(NULL), autoWrite(false)
{
    setFileName(fileName); // will also init `file` instance variable
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

void FileWriter::receiveData(DataVector d) {
    foreach(DataType item, d) {
        // TODO: actual formatting
        waitingQueue.enqueue( QString("%1\n").arg(item) );
    }
    emit queueSizeChanged(waitingQueue.size());

    if (autoWrite) {
        writeNow();
    }
}

void FileWriter::setAutoWriteEnabled(bool enabled) {
    autoWrite = enabled;
    if (enabled) {
        // TODO: check if can write and disable autoWrite if cannot
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
    int itemsWritten = 0;
    while ( ! writeQueue.isEmpty() ) {
        out << writeQueue.dequeue();
        itemsWritten += 1;
    }
    Logger::trace(tr("Written %1 items to file %2").arg(itemsWritten).arg(file->fileName()));
}

void FileWriter::openIfClosed() {
    if (file == NULL) { return; } // TODO: report internal error

    if ( ! file->isOpen()) {
        bool success = file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        if ( ! success ) {
            Logger::error(tr("Failed to open file %1: %2").arg(file->fileName()).arg(file->errorString()));
            return; // TODO: report error
        } else {
            Logger::info(tr("Opened file %1").arg(file->fileName()));
        }
    }
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
