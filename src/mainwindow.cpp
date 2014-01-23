#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "protocols/testprotocol.h"
#include "protocols/serialprotocol.h"
#include <QTimer>
#include "logger.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_draw.h"

namespace {
    const QString TEST_PROTOCOL = "TEST";
    const int DEFAULT_POINTS = 200;

    const QColor GRID_COLOR(128, 128, 128);
    const QColor CURVE_COLOR(40, 90, 120);
    const QColor CURVE_FILL(10, 160, 255, 100);
    const QColor AVERAGE_COLOR(255, 50, 50);

    void initGrid(QwtPlot *plot) {
        QwtPlotGrid * grid = new QwtPlotGrid;
        grid->enableXMin(true);
        grid->enableYMin(true);
        grid->setMajorPen(QPen(GRID_COLOR));
        grid->setMinorPen(QPen(GRID_COLOR, 1, Qt::DashLine));
        grid->attach(plot);
    }
    inline void rotateAxisLabel(QwtScaleDraw * axisDraw) {
        axisDraw->setLabelRotation(-90);
        axisDraw->setLabelAlignment(Qt::AlignHCenter | Qt::AlignTop);
    }
    QVector<QPointF> seriesData(DataVector data) {
        QVector<QPointF> res;
        int i = 0;
        foreach(DataType val, data) {
            res << QPointF(i, val);
            ++i;
        }
        return res;
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), protocol(NULL)
{
    ui->setupUi(this);
    worker = new Worker(NULL, this);

    setup();
}

void MainWindow::setup() {
    // Init GUI
    ui->portChooser->addItem(TEST_PROTOCOL);
    ui->portChooser->addItems(SerialProtocol::portNames());
    ui->pointsCounter->setValue(DEFAULT_POINTS);

    // Init plot(s)
    initPlot(ui->plotArea2); // currently unused
    initPlot(ui->plotArea3); // currently unused
    initPlot(ui->plotArea);

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

        QString portName = ui->portChooser->currentText();
        if(portName == TEST_PROTOCOL) {
            // An option for testing
            protocol = new TestProtocol(5, 100, this);
        } else {
            protocol = new SerialProtocol(portName, ui->pointsCounter->value(), this);
        }

        worker->reset(protocol);
        initWorkerHandlers();
        worker->prepare();
    });
}

void MainWindow::initWorkerHandlers() {
    worker->disconnect();
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
        ui->timeElapsed->setTime(QTime(0,0,0));
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
        // Update plot
        curve->setSamples(seriesData(worker->data()));
        ui->plotArea->replot();
    });
    connect(ui->stopBtn, &QPushButton::clicked, [=](){
        ui->stopBtn->setDisabled(true);
        ui->startBtn->setEnabled(true);
        ui->startBtn->setFocus();

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

void MainWindow::initPlot(QwtPlot *plot) {
    // FIXME: avoid max height
    plot->setMaximumHeight(180);

    rotateAxisLabel(plot->axisScaleDraw(QwtPlot::yLeft));
    initGrid(plot);
    // TODO: tooltip

    // TODO: different curves for different plots
    curve = new QwtPlotCurve;
    curve->setBrush(CURVE_FILL);
    curve->setPen(CURVE_COLOR);
    curve->setOrientation(Qt::Vertical);
    curve->attach(plot);
}

void MainWindow::log(QString text) {
    Logger::info(text);
}

MainWindow::~MainWindow()
{
    delete ui;
}
