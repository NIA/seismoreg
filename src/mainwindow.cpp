#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "protocols/testprotocol.h"
#include <QTimer>
#include "logger.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // TODO choosing port
    protocol = new TestProtocol(5, 100, this);
    worker = new Worker(protocol, this);

    setup();
}

void MainWindow::setup() {
    // Init GUI
    ui->portChooser->addItem(tr("TEST"));
    ui->ledGPS->setOnColor(QLed::Green);
    clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, [=](){
        QTime elapsed = QTime(0,0,0).addSecs(startedAt.secsTo(QDateTime::currentDateTime()));
        ui->timeElapsed->setTime(elapsed);
    });

    // Connect event handlers
    connect(ui->connectBtn, &QPushButton::clicked, [=](){
        ui->connectBtn->setDisabled(true);
        ui->disconnectBtn->setEnabled(true);
        ui->portChooser->setFocus();
        worker->reset(protocol);
        worker->prepare();
    });
    connect(worker->protocol(), &Protocol::checkedADC, [=](bool success){
        ui->ledADC->setOnColor( success ? QLed::Green : QLed::Red);
        ui->ledADC->setValue(true);
    });
    connect(worker->protocol(), &Protocol::checkedGPS, [=](bool success){
        ui->ledGPS->setOnColor( success ? QLed::Green : QLed::Red);
        ui->ledGPS->setValue(true);
    });
    connect(worker, &Worker::prepareFinished, [=](Worker::PrepareResult res){
        if(res == Worker::PrepareSuccess) {
            ui->startBtn->setEnabled(true);
            ui->startBtn->setFocus();
        } else {
            ui->connectBtn->setEnabled(true);
            ui->disconnectBtn->setDisabled(true);
        }
    });
    connect(ui->startBtn, &QPushButton::clicked, [=](){
        ui->startBtn->setDisabled(true);
        ui->stopBtn->setEnabled(true);
        ui->stopBtn->setFocus();

        startedAt = QDateTime::currentDateTime();
        ui->timeStart->setDateTime(startedAt);
        clockTimer->start(1000);

        if(worker->isStarted()) {
            worker->unpause();
        } else {
            worker->start();
        }
    });
    connect(worker, &Worker::dataUpdated, [=](DataVector d){
        QStringList items;
        foreach(DataType item, d) {
            items << QString::number(item);
        }
        ui->samplesRcvd->setText(QString::number(worker->data().size()));
        ui->dataView->addItems(items);
        ui->dataView->scrollToBottom();
    });
    connect(ui->stopBtn, &QPushButton::clicked, [=](){
        ui->stopBtn->setDisabled(true);
        ui->startBtn->setEnabled(true);

        worker->pause();
        clockTimer->stop();
    });
    connect(ui->disconnectBtn, &QPushButton::clicked, [=](){
        worker->finish();
        clockTimer->stop();

        ui->ledADC->setValue(false);
        ui->ledGPS->setValue(false);
        ui->disconnectBtn->setDisabled(true);
        ui->stopBtn->setDisabled(true);
        ui->startBtn->setDisabled(true);
        ui->connectBtn->setEnabled(true);
        ui->portChooser->setFocus();
    });
}

void MainWindow::log(QString text) {
    Logger::info(text);
}

MainWindow::~MainWindow()
{
    delete ui;
}
