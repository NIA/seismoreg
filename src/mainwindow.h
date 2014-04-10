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

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void autoWriteChanged(bool enabled);

private slots:
    void onDataReceived(TimeStampsVector t, DataVector d);
    void onFileNameChanged();

private:
    void setup();
    void initWorkerHandlers();
    void initFileHandlers();
    void initPortSettingsAction(QAction * action, QString title, PortSettingsEx & portSettings, QToolButton *btn);
    void setFileControlsState();
    void setCurrentTime();
    void log(QString text);

    void saveSettings();

    Ui::MainWindow *ui;
    Protocol * protocolADC;
    Protocol * protocolGPS;
    PortSettingsEx portSettingsADC;
    PortSettingsEx portSettingsGPS;
    Worker * worker;
    FileWriter * fileWriter;

    QDateTime startedAt;
    QTimer * clockTimer;
    int receivedItems;

    TimePlot * plots[CHANNELS_NUM];
    StatsBox * stats[CHANNELS_NUM];

    QVector<QWidget*> disableOnConnect;

    // Debugging and profiling:
    PerformanceReporter perfPlotting;
    PerformanceReporter perfWritting;
    PerformanceReporter perfTotal;
};

#endif // MAINWINDOW_H
