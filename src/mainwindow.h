#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include "protocol.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
    Protocol * protocol;
    int samplesReceived;
    QDateTime startedAt;
    QTimer * clockTimer;

    void setup();
    void log(QString text);
};

#endif // MAINWINDOW_H
