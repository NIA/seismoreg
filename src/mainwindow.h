#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include "protocol.h"
#include "worker.h"

namespace Ui {
class MainWindow;
}
class QwtPlotCurve;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void initPlot();
    
private:
    Ui::MainWindow *ui;
    Protocol * protocol;
    Worker * worker;

    QDateTime startedAt;
    QTimer * clockTimer;

    QwtPlotCurve * curve;

    void setup();
    void initWorkerHandlers();
    void log(QString text);
};

#endif // MAINWINDOW_H
