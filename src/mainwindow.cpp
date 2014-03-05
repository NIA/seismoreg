#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QFileDialog>

#include "protocols/testprotocol.h"
#include "protocols/serialprotocol.h"
#include "logger.h"
#include "settings.h"
#include "gui/statsbox.h"
#include "gui/timeplot.h"

namespace {
    const QString TEST_PROTOCOL = "TEST";
    const int FREQ_200 = 200;
    const int FREQ_50  = 50;

    const QString DEFAULT_FILENAME = QString("data-%1.dat").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss"));

    void initPortChooser(QComboBox * chooser, QString initialValue) {
        chooser->addItem(TEST_PROTOCOL);
        chooser->addItems(SerialProtocol::portNames());
        chooser->lineEdit()->setText(initialValue);
    }
    void initShowHideAction(QAction * action, QWidget * widgetToHide, bool initiallyShown) {
        action->setChecked(initiallyShown);
        widgetToHide->setVisible(initiallyShown);
        QObject::connect(action, &QAction::triggered, [=](bool checked){
            widgetToHide->setVisible(checked);
        });
    }
    template <class W>
    void initWidgetsArray(W * widgets[CHANNELS_NUM], W * w0, W * w1, W * w2) {
        widgets[0] = w0;
        widgets[1] = w1;
        widgets[2] = w2;
        // TODO: avoid this limitation
        Q_ASSERT_X(CHANNELS_NUM == 3, "MainWindow::initWidgetsArray", "MainWindow implementation assumes CHANNELS_NUM == 3");
    }

}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), protocol(NULL)
{
    ui->setupUi(this);
    worker = new Worker(NULL, this);
    fileWriter = new FileWriter(DEFAULT_FILENAME, this);

    initWidgetsArray(plots, ui->plotArea, ui->plotArea2, ui->plotArea3);
    initWidgetsArray(stats, ui->stats, ui->stats2, ui->stats3);

    setup();
}

void MainWindow::setup() {
    Settings settings;
    // Init GUI
    initPortChooser(ui->portChooser, settings.portNameADC());
    initPortChooser(ui->portChooserGPS, settings.portNameGPS());
    ui->samplingFreq->addItem(QString::number(FREQ_200));
    ui->samplingFreq->addItem(QString::number(FREQ_50));
    initFileHandlers();
    disableOnConnect << ui->portChooser << ui->portChooserGPS << ui->samplingFreq;

    initShowHideAction(ui->actionShowTable,    ui->dataView, settings.isTableShown());
    initShowHideAction(ui->actionShowSettings, ui->settings, settings.isSettingsShown());
    for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
        initShowHideAction(ui->actionShowStats, stats[ch], settings.isStatsShown());
    }

    ui->ledGPS->setOnColor(QLed::Green);
    clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, [=](){
        setCurrentTime();
        if(worker->isStarted() && ! worker->isPaused()) {
            QTime elapsed = QTime(0,0,0).addSecs(startedAt.secsTo(QDateTime::currentDateTime()));
            ui->timeElapsed->setTime(elapsed);
        }
    });
    clockTimer->start(1000);
    setCurrentTime(); // And set for the first time

    // Configure toolbar and status bar
    ui->mainToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    connect(Logger::instance(), &Logger::si_messageAdded, [=](Logger::Level level, QString message){
        if (level >= Logger::Info) {
            ui->statusBar->showMessage(message);
        }
    });

    // Connect event handlers
    connect(ui->connectBtn, &QPushButton::clicked, [=](){
        ui->connectBtn->setDisabled(true);
        ui->disconnectBtn->setEnabled(true);
        ui->portChooser->setFocus(); // TODO: control focus somehow another way
        foreach(QWidget * w, disableOnConnect) {
            w->setDisabled(true);
        }

        QString portName = ui->portChooser->currentText();
        int samplingFrequency = ui->samplingFreq->currentText().toInt();
        if(portName == TEST_PROTOCOL) {
            // An option for testing
            protocol = new TestProtocol(samplingFrequency, 9000000, this);
        } else {
            protocol = new SerialProtocol(portName, samplingFrequency, this);
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
            foreach(QWidget * w, disableOnConnect) {
                w->setEnabled(true);
            }
            ui->portChooser->setFocus();
        }
    });
    connect(ui->startBtn, &QPushButton::clicked, [=](){
        ui->startBtn->setDisabled(true);
        ui->stopBtn->setEnabled(true);
        ui->stopBtn->setFocus();

        startedAt = QDateTime::currentDateTime();
        ui->timeStart->setDateTime(startedAt);
        ui->timeElapsed->setTime(QTime(0,0,0));

        if(worker->isStarted()) {
            worker->unpause();
        } else {
            worker->start();
            setFileControlsState();
        }
    });
    connect(worker, &Worker::dataUpdated, [=](TimeStampsVector t, DataVector d){
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
        for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
            // Update plot
            plots[ch]->setData(worker->timeStamps(), worker->data(), ch);
            // Update stats
            stats[ch]->setStats(d, ch);
        }
    });
    connect(worker, &Worker::dataUpdated, fileWriter, &FileWriter::receiveData);

    connect(ui->stopBtn, &QPushButton::clicked, [=](){
        ui->stopBtn->setDisabled(true);
        ui->startBtn->setEnabled(true);
        ui->startBtn->setFocus();

        worker->pause();
        setFileControlsState();
    });
    connect(ui->disconnectBtn, &QPushButton::clicked, [=](){
        worker->finish();

        ui->ledADC->setValue(false);
        ui->ledGPS->setValue(false);
        ui->disconnectBtn->setDisabled(true);
        ui->stopBtn->setDisabled(true);
        ui->startBtn->setDisabled(true);
        ui->connectBtn->setEnabled(true);
        foreach(QWidget * w, disableOnConnect) {
            w->setEnabled(true);
        }
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

void MainWindow::setCurrentTime() {
    QDateTime now = QDateTime::currentDateTime();
    ui->currentDate->setText(now.date().toString(Qt::DefaultLocaleShortDate));
    ui->currentTime->setText(now.time().toString(Qt::DefaultLocaleShortDate));
}


void MainWindow::log(QString text) {
    Logger::info(text);
}

MainWindow::~MainWindow()
{
    clockTimer->stop();
    Settings settings;
    settings.setPortNameADC(ui->portChooser->currentText());
    settings.setPortNameGPS(ui->portChooserGPS->currentText());
    settings.setTableShown(ui->actionShowTable->isChecked());
    settings.setSettingsShown(ui->actionShowSettings->isChecked());
    settings.setStatsShown(ui->actionShowStats->isChecked());
    delete ui;
    delete worker;
    delete fileWriter;
}
