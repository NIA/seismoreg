#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QVector>
#include "protocol.h"
#include "protocols/serialprotocol.h"
#include "worker.h"
#include "filewriter.h"
#include "performancereporter.h"

namespace Ui {
class MainWindow;
}
class StatsBox;
class TimePlot;
class QToolButton;
class QThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    // Signals for multithreaded communication (with Worker, FileWriter)
    void protocolsChanged(ProtocolCreator * protADC, ProtocolCreator * protGPS);
    void preparingToStart(bool autostart);
    void starting();
    void stopping();
    void finishing();
    void fileNameChanged(QString outputDir, QString saveFileFormat);
    void autoWriteChanged(bool enabled);
    void finishingFile();
    void frequenciesSet(int samplingFreq, int filterFreq);
    void deviceIdSet(int id);

private slots:
    void onCheckedADC(bool success);
    void onCheckedGPS(bool success);
    void onPrepareFinished(Worker::PrepareResult res);
    void onTimeAvailable(QDateTime timeGPS);
    void onPositionAvailable(double latitiude, double longitude, double altitude);
    void onDataReceived(TimeStampsVector t, DataVector d);
    void onFileNameChanged();
    void setFixedScale();
    void onZoomChanged(double newMin, double newMax);
    void onLogMessage(Logger::Level level, QString message);
    void onQueueSizeChanged(unsigned size);
    void onStartedOrStopped(bool workerStarted);

private:
    void setup();
    void initWorkerHandlers();
    void initFileHandlers();
    void initPortSettingsAction(QAction * action, QString title, PortSettingsEx & portSettings, QToolButton *btn);
    void initZoomAction(QAction * action, QToolButton *btn);
    void setFileControlsState();
    void setCurrentTime();
    void log(QString text);
    void setReceivedItems(int received);
    void resetHistory();

    void saveSettings();

    Ui::MainWindow *ui;
    PortSettingsEx portSettingsADC;
    PortSettingsEx portSettingsGPS;
    Worker * worker;
    FileWriter * fileWriter;
    bool workerStarted;

    // Threads where FileWriter and Worker work
    QThread * threadFileWriter;
    QThread * threadWorker;

    QDateTime startedAt;
    QTimer * clockTimer;
    QDateTime synchronizedAt;
    int receivedItems;

    TimePlot * plots[CHANNELS_NUM];
    StatsBox * stats[CHANNELS_NUM];

    QVector<QWidget*> disableOnConnect;
    QVector<QWidget*> disableOnStart;

    // Debugging and profiling:
    PerformanceReporter perfStats;
    PerformanceReporter perfDataView;
    PerformanceReporter perfPlotting;
    PerformanceReporter perfTotal;
};

#endif // MAINWINDOW_H
