#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include "protocol.h"
#include "worker.h"
#include "filewriter.h"

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
    void initPlot(QwtPlot *plot);
    void setup();
    void initWorkerHandlers();
    void initFileHandlers();
    void setFileControlsState();
    void log(QString text);

    Ui::MainWindow *ui;
    Protocol * protocol;
    Worker * worker;
    FileWriter * fileWriter;

    QDateTime startedAt;
    QTimer * clockTimer;

    QwtPlotCurve * curve;
};

#endif // MAINWINDOW_H
