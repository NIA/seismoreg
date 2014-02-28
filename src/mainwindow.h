#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QVector>
#include "protocol.h"
#include "worker.h"
#include "filewriter.h"
#include "gui/statsbox.h"

namespace Ui {
class MainWindow;
}
class QwtPlotCurve;
class QwtPlot;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void autoWriteChanged(bool enabled);

private:
    void initPlot(int ch);
    void setup();
    void initWorkerHandlers();
    void initFileHandlers();
    void setFileControlsState();
    void setCurrentTime();
    void log(QString text);

    Ui::MainWindow *ui;
    Protocol * protocol;
    Worker * worker;
    FileWriter * fileWriter;

    QDateTime startedAt;
    QTimer * clockTimer;

    QwtPlotCurve * curves[CHANNELS_NUM];
    QwtPlot * plots[CHANNELS_NUM];
    StatsBox * stats[CHANNELS_NUM];

    QVector<QWidget*> disableOnConnect;
};

#endif // MAINWINDOW_H
