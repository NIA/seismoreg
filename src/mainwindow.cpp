#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "protocols/testprotocol.h"
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    protocol = new TestProtocol(5, 100, this);

    setup();
}

void MainWindow::setup() {
    // Init GUI
    ui->portChooser->addItem(tr("TEST"));
    ui->ledADC->setOnColor(QLed::Green);
    ui->ledGPS->setOnColor(QLed::Green);
    ui->samplesRcvd->setText(QString::number(samplesReceived));
    clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, [=](){
        QTime elapsed = QTime(0,0,0).addSecs(startedAt.secsTo(QDateTime::currentDateTime()));
        ui->timeElapsed->setTime(elapsed);
    });

    // Init behavior
    connect(ui->connectBtn, &QPushButton::clicked, [=](){
        QString portName = ui->portChooser->currentText();
        log(tr("    Opening port %1...").arg(portName));
        if(protocol->open()) {
            log(tr("[+] Opened port %1").arg(portName));
            ui->connectBtn->setEnabled(false);
            ui->portChooser->setFocus();
            log(tr("    Checking ADC..."));
            protocol->checkADC();
        } else {
            log(tr("[-] Failed to open port %1!").arg(portName));
        }
    });
    connect(protocol, &Protocol::checkedADC, [=](bool success){
        if(success) {
            log(tr("[+] ADC ready"));
            ui->ledADC->setValue(true);
            log(tr("    Checking GPS..."));
            protocol->checkGPS();
        } else {
            log(tr("[-] ADC check failed!"));
        }
    });
    connect(protocol, &Protocol::checkedGPS, [=](bool success){
        if(success) {
            log(tr("[+] GPS ready"));
            ui->ledGPS->setValue(true);
            log(tr("    Now you can start data receiving"));
            ui->startBtn->setEnabled(true);
            ui->startBtn->setFocus();
        } else {
            log(tr("[-] GPS check failed!"));
        }
    });
    connect(ui->startBtn, &QPushButton::clicked, [=](){
        ui->startBtn->setEnabled(false);
        ui->stopBtn->setEnabled(true);
        ui->stopBtn->setFocus();
        log(tr("[ ] Starting receiving data..."));
        startedAt = QDateTime::currentDateTime();
        ui->timeStart->setDateTime(startedAt);
        clockTimer->start(1000);
        protocol->startReceiving();
    });
    connect(protocol, &Protocol::dataAvailable, [=](QVector<DataType> d){
        int received = d.size();
        log(tr("[+] Received %1 data items").arg(received));
        samplesReceived += received;
        ui->samplesRcvd->setText(QString::number(samplesReceived));
        QStringList items;
        foreach(DataType item, d) {
            items << QString::number(item);
        }
        ui->dataView->addItems(items);
        ui->dataView->scrollToBottom();
    });
    connect(ui->stopBtn, &QPushButton::clicked, [=](){
        ui->stopBtn->setEnabled(false);
        ui->startBtn->setEnabled(true);
        log(tr("[ ] Stopped receiving data"));
        protocol->stopReceiving();
        clockTimer->stop();
    });
}

void MainWindow::log(QString text) {
    ui->logView->appendPlainText(text);
}

MainWindow::~MainWindow()
{
    delete ui;
}
