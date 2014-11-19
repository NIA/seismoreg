#include "filewriter.h"

#include "logger.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

const QString FileWriter::DEFAULT_OUTPUT_DIR = ".";
const QString FileWriter::DEFAULT_FILENAME_FORMAT = "%D%M%Y-%h%m%s-%f.w%i";
PerformanceReporter  FileWriter::perfReporter("FileWriter");

FileWriter::FileWriter(QString outputDirectory, QString fileNameFormat, QObject *parent) :
    QObject(parent), file(NULL), autoWrite(false),
    deviceID("00"), samplingFreq(0), filterFreq(0),
    latitude("???"), longitude("???"), itemsInQueue(0)
{
    setFileName(outputDirectory, fileNameFormat); // will also init `file` instance variable
}

inline QString twoDigitStr(int value) {
    return QString::number(value).rightJustified(2, '0');
}

QString FileWriter::buildFileName() const {
    const QDateTime &time = (startTime.isValid() ? startTime : QDateTime::currentDateTime());
    QString fileName = fileNameFormat();
    fileName.replace("%Y", QString::number(time.date().year()));
    fileName.replace("%M", twoDigitStr(time.date().month()));
    fileName.replace("%D", twoDigitStr(time.date().day()));
    fileName.replace("%h", twoDigitStr(time.time().hour()));
    fileName.replace("%m", twoDigitStr(time.time().minute()));
    fileName.replace("%s", twoDigitStr(time.time().second()));
    fileName.replace("%f", QString::number(filterFreq));
    fileName.replace("%r", QString::number(samplingFreq));
    fileName.replace("%i", deviceID);
    return outputDir + "/" + fileName;
}

QString FileWriter::fileNameFormatHelp() {
    return tr("%Y - year\n"
              "%M - month\n"
              "%D - day\n"
              "%h - hours\n"
              "%m - minutes\n"
              "%s - seconds\n"
              "%f - filter frequency\n"
              "%r - sampling frequency (rate)\n"
              "%i - device id");
}

void FileWriter::setFileName(QString outputDirectory, QString fileNameFormat) {
    if ((outputDirectory == outputDir) && (fileNameFormat == fileFormat)) {
        return; // Nothing changed
    }
    outputDir = outputDirectory;
    fileFormat = fileNameFormat;
    closeIfOpened();
}

void FileWriter::receiveData(TimeStampsVector t, DataVector d) {
    perfReporter.start();
    int count = qMin(t.size(), d.size()); // TODO: warn if different or empty
    if (count == 0) { return; }

    if (startTime.isNull()) { // Not set yet
        startTime = QDateTime::fromMSecsSinceEpoch(t.first());
    }

    QByteArray allData;
    QByteArray number;
    foreach(const DataItem & item, d) {
        for(unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
            number.setNum(item.byChannel[ch]);
            allData += number;
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
    perfReporter.stop();
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

void FileWriter::setDeviceID(QString id)
{
    deviceID = id;
    // For debug:
    Logger::info(tr("Device id: %1").arg(id));
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
            }// TODO: else: do what? Append - incorrect, rewrite - should ask
            return true;
        }
    }
    return true;
}

void FileWriter::writeHeader() {
    QTextStream out(file);
    out << QStringLiteral("[Description]\n");
    out << tr("Device ID=%1").arg(deviceID);
    out << QStringLiteral("\n[Frequency]\n");
    out << tr("%1 Hz").arg(filterFreq);
    out << QStringLiteral("\n[Sample rate]\n");
    out << tr("%1 Hz").arg(samplingFreq);
    out << QStringLiteral("\n[Time]\n%1\n").arg(startTime.toString("hh:mm:ss"));
    out << QStringLiteral("[Coordinates]\n");
    out << tr("%1 deg. - latitude\n").arg(latitude);
    out << tr("%1 deg. - longitude\n").arg(longitude);
    out << QStringLiteral("[Date]\n%1\n").arg(startTime.toString("dd.MM.yyyy"));
    out << QStringLiteral("[Values]\n");
}

void FileWriter::closeIfOpened() {
    waitingQueue.clear();
    itemsInQueue = 0;
    emit queueSizeChanged(itemsInQueue);
    startTime = QDateTime(); // set null datetime so that it will be reset next time
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
