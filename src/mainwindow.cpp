#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "protocols/testprotocol.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    protocol = new TestProtocol(5, 100, this);

    setup();
}

void MainWindow::setup() {
    ui->portChooser->addItem(tr("TEST"));

    connect(ui->connectBtn, &QPushButton::clicked, [=](){
        QString portName = ui->portChooser->currentText();
        log(tr("    Opening port %1...").arg(portName));
        if(protocol->open()) {
            log(tr("[+] Opened port %1").arg(portName));
            ui->connectBtn->setEnabled(false);
            log(tr("    Checking ADC..."));
            protocol->checkADC();
        } else {
            log(tr("[-] Failed to open port %1!").arg(portName));
        }
    });
    connect(protocol, &Protocol::checkedADC, [=](bool success){
        if(success) {
            log(tr("[+] ADC ready"));
            ui->readyADC->setChecked(true);
            log(tr("    Checking GPS..."));
            protocol->checkGPS();
        } else {
            log(tr("[-] ADC check failed!"));
        }
    });
    connect(protocol, &Protocol::checkedGPS, [=](bool success){
        if(success) {
            log(tr("[+] GPS ready"));
            ui->readyGPS->setChecked(true);
            log(tr("    Now you can start data receiving"));
            ui->startBtn->setEnabled(true);
        } else {
            log(tr("[-] GPS check failed!"));
        }
    });
    connect(ui->startBtn, &QPushButton::clicked, [=](){
        ui->startBtn->setEnabled(false);
        ui->stopBtn->setEnabled(true);
        log(tr("[ ] Starting receiving data..."));
        protocol->startReceiving();
    });
    connect(protocol, &Protocol::dataAvailable, [=](QVector<DataType> d){
        log(tr("[+] Received %1 data items").arg(d.size()));
        QStringList items;
        foreach(DataType item, d) {
            items << QString::number(item);
        }
        ui->dataView->addItems(items);
    });
    connect(ui->stopBtn, &QPushButton::clicked, [=](){
        ui->stopBtn->setEnabled(false);
        ui->startBtn->setEnabled(true);
        log(tr("[ ] Stopped receiving data"));
        protocol->stopReceiving();
    });
}

void MainWindow::log(QString text) {
    ui->logView->appendPlainText(text);
}

MainWindow::~MainWindow()
{
    delete ui;
}
