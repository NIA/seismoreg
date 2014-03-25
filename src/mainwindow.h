#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QVector>
#include "qextserialport.h"
#include "protocol.h"
#include "worker.h"
#include "filewriter.h"

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

private:
    void setup();
    void initWorkerHandlers();
    void initFileHandlers();
    void initPortSettingsAction(QAction * action, QString title, PortSettings & portSettings, QToolButton *btn);
    void setFileControlsState();
    void setCurrentTime();
    void log(QString text);

    void saveSettings();

    Ui::MainWindow *ui;
    Protocol * protocolADC;
    Protocol * protocolGPS;
    PortSettings portSettingsADC;
    PortSettings portSettingsGPS;
    Worker * worker;
    FileWriter * fileWriter;

    QDateTime startedAt;
    QTimer * clockTimer;
    int receivedItems;

    TimePlot * plots[CHANNELS_NUM];
    StatsBox * stats[CHANNELS_NUM];

    QVector<QWidget*> disableOnConnect;
};

#endif // MAINWINDOW_H
