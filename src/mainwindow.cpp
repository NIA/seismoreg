#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "protocols/testprotocol.h"
#include "protocols/serialprotocol.h"
#include "logger.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_draw.h"

#include <QTimer>
#include <QFileDialog>

namespace {
    const QString TEST_PROTOCOL = "TEST";
    const int DEFAULT_POINTS = 200;

    const QColor GRID_COLOR(128, 128, 128);
    const QColor CURVE_COLOR(40, 90, 120);
    const QColor CURVE_FILL(10, 160, 255, 100);
    const QColor AVERAGE_COLOR(255, 50, 50);

    const QString DEFAULT_FILENAME = QString("data-%1.dat").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss"));

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
    QVector<QPointF> seriesData(DataVector data, unsigned ch) {
        QVector<QPointF> res;
        int i = 0;
        foreach(DataItem item, data) {
            res << QPointF(i, item.byChannel[ch]);
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
    fileWriter = new FileWriter(DEFAULT_FILENAME, this);

    plots[0] = ui->plotArea;
    plots[1] = ui->plotArea2;
    plots[2] = ui->plotArea3;
    // TODO: avoid this limitation
    Q_ASSERT_X(CHANNELS_NUM == 3, "MainWindow::MainWindow", "MainWindow implementation assumes CHANNELS_NUM == 3");

    setup();
}

void MainWindow::setup() {
    // Init GUI
    ui->portChooser->addItem(TEST_PROTOCOL);
    ui->portChooser->addItems(SerialProtocol::portNames());
    ui->pointsCounter->setValue(DEFAULT_POINTS);
    initFileHandlers();

    // Init plot(s)
    for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
        initPlot(ch);
    }

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
            setFileControlsState();
        }
    });
    connect(worker, &Worker::dataUpdated, [=](DataVector d){
        QStringList items;
        foreach(DataItem item, d) {
            // TODO: use table instead of list
            QStringList itemStr;
            for(unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
                itemStr << QString::number(item.byChannel[ch]);
            }
            items << itemStr.join("; ");
        }
        ui->samplesRcvd->setText(QString::number(worker->data().size()));
        ui->dataView->addItems(items);
        ui->dataView->scrollToBottom();
        // Update plot
        for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
            curves[ch]->setSamples(seriesData(worker->data(), ch));
            plots[ch]->replot();
        }
    });
    connect(worker, &Worker::dataUpdated, fileWriter, &FileWriter::receiveData);

    connect(ui->stopBtn, &QPushButton::clicked, [=](){
        ui->stopBtn->setDisabled(true);
        ui->startBtn->setEnabled(true);
        ui->startBtn->setFocus();

        worker->pause();
        clockTimer->stop();
        setFileControlsState();
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
        setFileControlsState();
    });
}

void MainWindow::initFileHandlers() {

    connect(this,             &MainWindow::autoWriteChanged, fileWriter, &FileWriter::setAutoWriteEnabled);
    connect(ui->saveFileName, &QLineEdit::textChanged,       fileWriter, &FileWriter::setFileName);
    connect(ui->writeNowBtn,  &QPushButton::clicked,         fileWriter, &FileWriter::writeOnce);
    // connecting to Worker::dataUpdated is made in initWorkerHandlers
    connect(fileWriter, &FileWriter::queueSizeChanged, [=](unsigned size){
        ui->samplesInQueue->setText(QString::number(size));
    });

    // TODO: if auto-write fails, worker should notify GUI (show warning, uncheck checkbox)
    connect(ui->writeToFileEnabled, &QCheckBox::stateChanged, [=](int state){
        bool enabled = (state == Qt::Checked);
        emit autoWriteChanged(enabled);
        setFileControlsState();
    });

    connect(ui->browseBtn, &QPushButton::clicked, [=](){
        QString file = QFileDialog::getSaveFileName(this, tr("Choose file for writing data"), ui->saveFileName->text());
        if( ! file.isEmpty()) {
            ui->saveFileName->setText(file);
            // this will automatically notify fileWriter: changing ui->saveFileName text
            // will trigger textChanged, which is connected to setFileName of FileWriter (see above)
        }
    });

    // Set initial values
    ui->saveFileName->setText(DEFAULT_FILENAME);
    emit autoWriteChanged(ui->writeToFileEnabled->isChecked());
    setFileControlsState();
}

void MainWindow::setFileControlsState() {
    bool disableChangingFile = (ui->writeToFileEnabled->isChecked() && worker->isStarted());
    // If running and auto-saving => cannot change file name
    ui->saveFileName->setDisabled(disableChangingFile);
    ui->browseBtn->setDisabled(disableChangingFile);

    bool disableWriteNow = (ui->writeToFileEnabled->isChecked());
    // If auto-saving => cannot write now
    ui->writeNowBtn->setDisabled(disableWriteNow);
}

void MainWindow::initPlot(int ch) {
    QwtPlot * plot = plots[ch];
    // FIXME: avoid max height
    plot->setMaximumHeight(180);

    rotateAxisLabel(plot->axisScaleDraw(QwtPlot::yLeft));
    initGrid(plot);
    // TODO: tooltip

    // TODO: different curves for different plots
    curves[ch] = new QwtPlotCurve;
    curves[ch]->setBrush(CURVE_FILL);
    curves[ch]->setPen(CURVE_COLOR);
    curves[ch]->setOrientation(Qt::Vertical);
    curves[ch]->attach(plot);
}

void MainWindow::log(QString text) {
    Logger::info(text);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete worker;
    delete fileWriter;
}
