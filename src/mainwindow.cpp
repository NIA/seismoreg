#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QList>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <qwt_scale_div.h>

#include "protocols/testprotocol.h"
#include "protocols/serialprotocol.h"
#include "logger.h"
#include "settings.h"
#include "system.h"
#include "gui/statsbox.h"
#include "gui/timeplot.h"
#include "gui/portsettingsdialog.h"

namespace {
    const QString TEST_PROTOCOL = "TEST";
    const int FREQ_200 = 200;
    const int FREQ_50  = 50;
    const int FREQ_10  = 10;
    const int FREQ_1   = 1;

    const int TIME_SYNC_PERIOD_SECS = 60; // sync time every minute
    const int NEW_FILE_PERIOD_SECS  = 60*60; // reset file every hour
    const int MAX_WAIT = 5000; // Wait background threads no more than 5 secs

    void initPortChooser(QComboBox * chooser, QString initialValue) {
        chooser->addItem(TEST_PROTOCOL);
        chooser->addItems(SerialProtocol::portNames());
        chooser->lineEdit()->setText(initialValue);
    }
    void initFreqChooser(QComboBox * chooser, QList<int> values, int initialValue) {
        foreach(int value, values) {
            chooser->addItem(QString::number(value));
            if (value == initialValue) {
                // select this (last) value
                chooser->setCurrentIndex(chooser->count() - 1);
            }
        }
    }
    void initFreqSlider(QwtSlider * slider, int minValue, int maxValue, int initialValue) {
        slider->setLowerBound(minValue);
        slider->setUpperBound(maxValue);
        // Actually allow only two positions: min and max
        slider->setTotalSteps(1);
        // Therefore, only two major ticks
        slider->setScale(QwtScaleDiv(minValue, maxValue, QList<double>(), QList<double>(), QList<double>({FREQ_50, FREQ_200})));
        slider->setValue(initialValue);
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
        Q_STATIC_ASSERT_X(CHANNELS_NUM == 3, "MainWindow::initWidgetsArray implementation assumes CHANNELS_NUM == 3");
    }

    // "Protocol factory"
    ProtocolCreator * makeProtocol(QString portName, int samplingFrequency, int filterFrequency, PortSettingsEx portSettings = SerialProtocol::DEFAULT_PORT_SETTINGS) {
        if(portName == TEST_PROTOCOL) {
            // An option for testing
            return new TestProtocolCreator(samplingFrequency, 9000000);
        } else {
            return new SerialProtocolCreator(portName, samplingFrequency, filterFrequency, portSettings);
        }
    }

}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), receivedItems(0),
    workerStarted(false),
    perfStats(tr("Stats")), perfDataView(tr("DataView")),
    perfPlotting(tr("Plotting")), perfTotal(tr("Total (MainWindow)"))
{
    ui->setupUi(this);
    fileWriter = new FileWriter(FileWriter::DEFAULT_OUTPUT_DIR, FileWriter::DEFAULT_FILENAME_FORMAT);
    /* TODO: this is the attempt to begin working on multithreading. There are still problems:
     *  - all interaction should be through signals, not only receiveData
     *  ~ [partially fixed] using Logger from another thread causes crash (signal is not queued)
     *  - some data may be lost (not written to file) when thread is finished
     *  - ...
     */
    threadFileWriter = new QThread;
    fileWriter->moveToThread(threadFileWriter);
    threadFileWriter->start();
    worker = new Worker;
    threadWorker = new QThread;
    worker->moveToThread(threadWorker);
    threadWorker->start(QThread::HighestPriority);

    initWidgetsArray(plots, ui->plotArea, ui->plotArea2, ui->plotArea3);
    initWidgetsArray(stats, ui->stats, ui->stats2, ui->stats3);

    setup();
}

void MainWindow::setup() {
    Settings settings;
    // Init connections
    initWorkerHandlers();
    initFileHandlers();
    // Configure toolbar and status bar
    ui->mainToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    connect(Logger::instance(), &Logger::si_messageAdded, this, &MainWindow::onLogMessage, Qt::QueuedConnection);

    // Init GUI
    initPortChooser(ui->portChooser, settings.portName(Settings::PortADC));
    initPortChooser(ui->portChooserGPS, settings.portName(Settings::PortGPS));
    initFreqChooser(ui->samplingFreq, QList<int>({FREQ_200, FREQ_50, FREQ_10, FREQ_1}), settings.samplingFrequency());
    initFreqSlider(ui->filterFreqSlider, FREQ_50, FREQ_200, settings.filterFrequency());
    disableOnConnect = { ui->portChooser,     ui->portChooserGPS,
                         ui->portSettingsADC, ui->portSettingsGPS };
    disableOnStart   = { ui->samplingFreq,    ui->filterFreqSlider };

    initShowHideAction(ui->actionShowTable,    ui->dataView, settings.isTableShown());
    initShowHideAction(ui->actionShowSettings, ui->settings, settings.isSettingsShown());
    for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
        initShowHideAction(ui->actionShowStats, stats[ch], settings.isStatsShown());

        plots[ch]->setChannel(ch);
    }
    ui->connectBtn->setFocus();

    // Plot settings
    initZoomAction(ui->actionZoomIn,    ui->zoomInBtn);
    initZoomAction(ui->actionZoomOut,   ui->zoomOutBtn);
    initZoomAction(ui->actionMoveUp,    ui->upBtn);
    initZoomAction(ui->actionMoveDown,  ui->downBtn);
    initZoomAction(ui->actionZoomReset, ui->resetZoomBtn);
    void (QSpinBox:: *valueChangedSignal)(int) = &QSpinBox::valueChanged; // resolve overloaded function
    for (TimePlot *plot:  plots) {
        connect(ui->fixedScale,      &QRadioButton::toggled, plot, &TimePlot::setFixedScaleY);
        connect(ui->fixedScaleMax,   valueChangedSignal,     plot, &TimePlot::setFixedScaleYMax);
        connect(ui->fixedScaleMin,   valueChangedSignal,     plot, &TimePlot::setFixedScaleYMin);
        connect(ui->timeInterval,    valueChangedSignal,     plot, &TimePlot::setHistorySecs);
        connect(ui->actionZoomIn,    &QAction::triggered,    plot, &TimePlot::zoomIn);
        connect(ui->actionZoomOut,   &QAction::triggered,    plot, &TimePlot::zoomOut);
        connect(ui->actionMoveUp,    &QAction::triggered,    plot, &TimePlot::moveUp);
        connect(ui->actionMoveDown,  &QAction::triggered,    plot, &TimePlot::moveDown);
        connect(ui->actionZoomReset, &QAction::triggered,    plot, &TimePlot::resetZoom);
        connect(ui->fixNowBtn,       &QPushButton::clicked,  plot, &TimePlot::fixCurrent);

        plot->setFixedScaleYMax(settings.plotFixedScaleMax());
        plot->setFixedScaleYMin(settings.plotFixedScaleMin());
        plot->setFixedScaleY(settings.isPlotFixedScale());
        plot->setHistorySecs(settings.plotHistorySecs());
    }
    connect(ui->fixedScaleMax, valueChangedSignal, this, &MainWindow::setFixedScale);
    connect(ui->fixedScaleMin, valueChangedSignal, this, &MainWindow::setFixedScale);
    connect(ui->fixNowBtn,  &QPushButton::clicked, this, &MainWindow::setFixedScale);
    // TODO: using plot[0] here is not quite great
    connect(plots[CHANNELS_NUM-1], &TimePlot::zoomChanged, this, &MainWindow::onZoomChanged);
    (settings.isPlotFixedScale() ? ui->fixedScale : ui->autoScale)->setChecked(true);
    ui->fixedScaleMax->setValue( settings.plotFixedScaleMax() );
    ui->fixedScaleMin->setValue( settings.plotFixedScaleMin() );
    ui->timeInterval->setValue(  settings.plotHistorySecs()   );

    portSettingsADC = settings.portSettigns(Settings::PortADC);
    portSettingsGPS = settings.portSettigns(Settings::PortGPS);
    initPortSettingsAction(ui->actionADCPortSettings, ui->actionADCPortSettings->text(), portSettingsADC, ui->portSettingsADC);
    initPortSettingsAction(ui->actionGPSPortSettings, ui->actionGPSPortSettings->text(), portSettingsGPS, ui->portSettingsGPS);

    ui->ledGPS->setOnColor(QLed::Green);
    clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, [=](){
        setCurrentTime();
        if(workerStarted) {
            QTime elapsed = QTime(0,0,0).addSecs(startedAt.secsTo(QDateTime::currentDateTime()));
            ui->timeElapsed->setTime(elapsed);
        }
    });
    clockTimer->start(1000);
    setCurrentTime(); // And set for the first time

    // Connect event handlers
    connect(ui->connectBtn, &QPushButton::clicked, [=](){
        ui->connectBtn->setDisabled(true);
        ui->disconnectBtn->setEnabled(true);
        foreach(QWidget * w, disableOnConnect) {
            w->setDisabled(true);
        }
        ui->ledADC->setValue(false);
        ui->ledGPS->setValue(false);

        // TODO: if frequencies are anyway set on start, do we need to pass them to constructor?
        int samplingFrequency = ui->samplingFreq->currentText().toInt();
        int filterFrequency   = ui->filterFreqSlider->value();

        QString portNameADC = ui->portChooser->currentText();
        QString portNameGPS = ui->portChooserGPS->currentText();
        ProtocolCreator * protocolCreatorADC = makeProtocol(portNameADC, samplingFrequency, filterFrequency, portSettingsADC);
        ProtocolCreator * protocolCreatorGPS = NULL;
        if (portNameADC == portNameGPS) {
            // Important! If port names are equal, protocols also should be the
            // same instance, NOT two different instances with the same parameters!
            protocolCreatorGPS = protocolCreatorADC;
            // TODO: move this `if` into makeProtocol and move this function to core?
        } else {
            protocolCreatorGPS = makeProtocol(portNameGPS, samplingFrequency, filterFrequency, portSettingsGPS);
        }

        // Calls Worker::reset
        emit protocolsChanged(protocolCreatorADC, protocolCreatorGPS);
        // Calls Worker::prepare(autostart = false)
        emit preparingToStart(false);
    });
    connect(ui->startBtn, &QPushButton::clicked, [=](){
        ui->startBtn->setDisabled(true);
        ui->stopBtn->setEnabled(true);
        ui->stopBtn->setFocus();
        foreach(QWidget * w, disableOnStart) {
            w->setDisabled(true);
        }
        ui->ledWorking->setOnColor(QLed::Green);
        ui->ledWorking->setValue(true);

        resetHistory();

        // Sampling frequency and filter frequency might have changed
        int samplingFrequency = ui->samplingFreq->currentText().toInt();
        int filterFrequency   = ui->filterFreqSlider->value();
        for(TimePlot * plot: plots) {
            plot->setPointsPerSec(samplingFrequency);
        }
        // Calls Worker::setFrequencies and FileWriter::setFrequencies
        emit frequenciesSet(samplingFrequency, filterFrequency);
        // Calls Worker::start, this will also trigger setFileControlsState
        emit starting();
    });
    connect(ui->stopBtn, &QPushButton::clicked, [=](){
        ui->stopBtn->setDisabled(true);
        ui->startBtn->setEnabled(true);
        ui->startBtn->setFocus();
        foreach(QWidget * w, disableOnStart) {
            w->setEnabled(true);
        }
        ui->ledWorking->setValue(false);

        // calls Worker::stop, this will also trigger setFileControlsState
        emit stopping();
    });
    connect(ui->disconnectBtn, &QPushButton::clicked, [=](){
        // calls Worker::finish, this will also trigger setFileControlsState
        emit finishing();
        ui->ledADC->setValue(false);
        ui->ledGPS->setValue(false);
        ui->disconnectBtn->setDisabled(true);
        ui->stopBtn->setDisabled(true);
        ui->startBtn->setDisabled(true);
        ui->connectBtn->setEnabled(true);
        foreach(QWidget * w, disableOnConnect+disableOnStart) {
            w->setEnabled(true);
        }
        ui->portChooser->setFocus();

        ui->ledWorking->setValue(false);
    });
    checkOutputDirectory();
}

void MainWindow::initWorkerHandlers() {
    // Incoming
    connect(worker, &Worker::checkedADC,        this, &MainWindow::onCheckedADC);
    connect(worker, &Worker::checkedGPS,        this, &MainWindow::onCheckedGPS);
    connect(worker, &Worker::timeAvailable,     this, &MainWindow::onTimeAvailable);
    connect(worker, &Worker::positionAvailable, this, &MainWindow::onPositionAvailable);
    connect(worker, &Worker::prepareFinished,   this, &MainWindow::onPrepareFinished);
    connect(worker, &Worker::dataUpdated,       this, &MainWindow::onDataReceived);
    connect(worker, &Worker::positionAvailable, fileWriter, &FileWriter::setCoordinates);
    connect(worker, &Worker::dataUpdated,       fileWriter, &FileWriter::receiveData);
    connect(worker, &Worker::startedOrStopped,  this, &MainWindow::onStartedOrStopped);

    // Outcoming
    connect(this, &MainWindow::protocolsChanged,   worker, &Worker::reset);
    connect(this, &MainWindow::preparingToStart,   worker, &Worker::prepare);
    connect(this, &MainWindow::starting,           worker, &Worker::start);
    connect(this, &MainWindow::stopping,           worker, &Worker::stop);
    connect(this, &MainWindow::finishing,          worker, &Worker::finish);
    connect(this, &MainWindow::frequenciesSet,     worker, &Worker::setFrequencies);
}

void MainWindow::initFileHandlers() {

    connect(this,               &MainWindow::autoWriteChanged, fileWriter, &FileWriter::setAutoWriteEnabled);
    connect(this,               &MainWindow::frequenciesSet,   fileWriter, &FileWriter::setFrequencies);
    connect(this,               &MainWindow::deviceIdSet,      fileWriter, &FileWriter::setDeviceID);
    connect(this,               &MainWindow::stopping,         fileWriter, &FileWriter::finishFile);
    connect(this,               &MainWindow::finishing,        fileWriter, &FileWriter::finishFile);
    connect(this,               &MainWindow::finishingFile,    fileWriter, &FileWriter::finishFile);
    connect(ui->outputDir,      &QLineEdit::editingFinished,   this,       &MainWindow::onFileNameChanged);
    connect(ui->saveFileFormat, &QLineEdit::editingFinished,   this,       &MainWindow::onFileNameChanged);
    connect(this,               &MainWindow::fileNameChanged,  fileWriter, &FileWriter::setFileName);
    connect(ui->writeNowBtn,    &QPushButton::clicked,         fileWriter, &FileWriter::writeOnce);
    // connecting to Worker::dataUpdated is made in initWorkerHandlers
    connect(fileWriter, &FileWriter::queueSizeChanged, this, &MainWindow::onQueueSizeChanged);

    // TODO: if auto-write fails, worker should notify GUI (show warning, uncheck checkbox)
    connect(ui->writeToFileEnabled, &QCheckBox::stateChanged, [=](int state){
        bool enabled = (state == Qt::Checked);
        emit autoWriteChanged(enabled);
        setFileControlsState();
    });

    connect(ui->browseBtn, &QPushButton::clicked, [=](){
        QString dir = QFileDialog::getExistingDirectory(this, tr("Choose output directory for data files"), ui->outputDir->text());
        if( ! dir.isEmpty()) {
            ui->outputDir->setText(dir);
            // this will automatically notify fileWriter: changing ui->saveFileName text
            // will trigger textChanged, which is connected to setFileName of FileWriter (see above)
        }
    });
    connect(ui->formatHelpBtn, &QPushButton::clicked, [=](){
        QMessageBox::information(this, tr("Filename format help"), FileWriter::fileNameFormatHelp());
    });

    // Set initial values
    Settings settings;
    ui->outputDir->setText(settings.outputDirectory());
    ui->saveFileFormat->setText(settings.fileNameFormat());
    emit fileNameChanged(settings.outputDirectory(), settings.fileNameFormat());
    emit deviceIdSet(settings.deviceId());
    emit autoWriteChanged(ui->writeToFileEnabled->isChecked());
    setFileControlsState();
}

void MainWindow::onCheckedADC(bool success) {
    ui->ledADC->setOnColor( success ? QLed::Green : QLed::Red);
    ui->ledADC->setValue(true);
}

void MainWindow::onCheckedGPS(bool success) {
    ui->ledGPS->setOnColor( success ? QLed::Green : QLed::Red);
    ui->ledGPS->setValue(true);
}

void MainWindow::onPrepareFinished(Worker::PrepareResult res) {
    if(res == Worker::PrepareSuccess) {
        ui->startBtn->setEnabled(true);
        ui->startBtn->setFocus();
    } else {
        ui->connectBtn->setEnabled(true);
        ui->disconnectBtn->setDisabled(true);
        foreach(QWidget * w, disableOnConnect) {
            w->setEnabled(true);
        }
        ui->connectBtn->setFocus();

        if (res == Worker::PrepareFailADC) {
            ui->ledADC->setOnColor(QLed::Red);
            ui->ledADC->setValue(true);
        } else if (res == Worker::PrepareFailGPS) {
            ui->ledGPS->setOnColor(QLed::Red);
            ui->ledGPS->setValue(true);
        }
    }
}

void MainWindow::onTimeAvailable(QDateTime time) {
    Logger::info(tr("Received time update: %1UTC").arg(time.toString("yyyy-MM-dd hh:mm:ss.zzz")));
    if (synchronizedAt.isNull() || synchronizedAt.secsTo(time) > TIME_SYNC_PERIOD_SECS) {
        // TODO: an option to disable changing system clock
        System::setSystemTime(time);
        synchronizedAt = time;
    }
    ui->ledGPS->blinkOnce();
}

void MainWindow::onPositionAvailable(double latitude, double longitude, double altitude) {
    Logger::info(tr("Received position update: %1, %2, %3m").arg(latitude).arg(longitude).arg(altitude));
    // TODO: convert to minutes/seconds format?
    QString latitudeStr  = QString::number(latitude, 'f', 6);
    QString longitudeStr = QString::number(longitude, 'f', 6);

    ui->currentLatitude->setText(latitudeStr);
    ui->currentLongitude->setText(longitudeStr);
    ui->ledGPS->blinkOnce();
}

void MainWindow::onDataReceived(TimeStampsVector t, DataVector d) {
    if (t.isEmpty() || d.isEmpty()) { return; }
    perfTotal.start();

    Logger::trace(tr("Received %1 data items").arg(d.size()*CHANNELS_NUM));

    if (startedAt.secsTo(QDateTime::fromMSecsSinceEpoch(t.last())) >= NEW_FILE_PERIOD_SECS) {
        // Maximum time for file elapsed, close file and open new one then
        emit finishingFile();
        resetHistory();
    }

    setReceivedItems(receivedItems + d.size()*CHANNELS_NUM);

    if (ui->actionShowTable->isChecked()) {
        perfDataView.start();
        QStringList items;
        QByteArray number;
        foreach(const DataItem & item, d) {
            QByteArray itemStr;
            for(unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
                number.setNum(item.byChannel[ch]);
                itemStr += number;
                itemStr += '\t';
            }
            items << itemStr;
        }
        ui->dataView->clear();
        ui->dataView->addItems(items);
        ui->dataView->scrollToBottom();
        perfDataView.stop();
    }

    perfStats.start();
    for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
        // Update stats
        stats[ch]->setStats(d, ch);
    }
    perfStats.stop();

    // TODO: don't call these slots (TimePlot::receiveData), connect them separately.
    perfPlotting.start();
    for(TimePlot * plot: plots) {
        plot->receiveData(t, d);
    }
    perfPlotting.stop();

    ui->ledADC->blinkOnce();

    perfTotal.stop();
}

void MainWindow::onLogMessage(Logger::Level level, QString message) {
    if (level >= Logger::Info) {
        ui->statusBar->showMessage(message);
    }
}

void MainWindow::onQueueSizeChanged(unsigned size) {
    ui->samplesInQueue->setText(QString::number(size));
}

void MainWindow::setReceivedItems(int received) {
    receivedItems = received;
    ui->samplesRcvd->setText(QString::number(receivedItems));
}

void MainWindow::resetHistory() {
    for(TimePlot * plot: plots) {
        plot->clearHistory();
    }
    setReceivedItems(0);
    startedAt = QDateTime::currentDateTime();
    ui->timeStart->setDateTime(startedAt);
    ui->timeElapsed->setTime(QTime(0,0,0));
}

void MainWindow::onFileNameChanged() {
    checkOutputDirectory();
    // Propagate the change to FileWriter
    emit fileNameChanged(ui->outputDir->text(), ui->saveFileFormat->text());
}

bool MainWindow::checkOutputDirectory() {
    QString dir = ui->outputDir->text();
    QFile testFile(dir + QString("/test%1.tmp").arg(QApplication::applicationPid()));
    bool writable = testFile.open(QFile::WriteOnly);
    if (writable) {
        testFile.close();
        testFile.remove();

        Logger::trace(tr("Current output directory '%1' is checked").arg(dir));
        // TODO: avoid repetition in working with LEDs
        ui->ledReady->setOnColor(QLed::Green);
        ui->ledReady->setValue(true);
    } else {
        Logger::warning(tr("Current output directory '%1' isn't writable!").arg(dir));
        ui->ledReady->setOnColor(QLed::Red);
        ui->ledReady->setValue(true);
    }
    return writable;
}

void MainWindow::initPortSettingsAction(QAction * action, QString title, PortSettingsEx & portSettings, QToolButton * btn) {
    btn->setDefaultAction(action);
    connect(action, &QAction::triggered, [=,&portSettings](){
        PortSettingsDialog dlg(title, portSettings, this);
        if (PortSettingsDialog::Accepted == dlg.exec()) {
            portSettings = dlg.portSettings();
        }
    });
}


void MainWindow::onStartedOrStopped(bool workerStarted) {
    this->workerStarted = workerStarted;
    setFileControlsState();
}


void MainWindow::initZoomAction(QAction *action, QToolButton *btn) {
    btn->setDefaultAction(action);
}

void MainWindow::setFixedScale() {
    ui->fixedScale->setChecked(true);
}

void MainWindow::onZoomChanged(double newMin, double newMax) {
    // TODO: really bad practice!
    ui->fixedScaleMax->blockSignals(true);
    ui->fixedScaleMin->blockSignals(true);
    ui->fixedScaleMin->setValue( int(newMin) );
    ui->fixedScaleMax->setValue( int(newMax) );
    ui->fixedScaleMax->blockSignals(false);
    ui->fixedScaleMin->blockSignals(false);

    setFixedScale();

    if (!workerStarted) {
        // replot to see the change (when started, it will be replotted when received next data)
        for (TimePlot * plot: plots) {
            plot->replot();
        }
    }
}

void MainWindow::setFileControlsState() {
    bool disableChangingFile = (ui->writeToFileEnabled->isChecked() && workerStarted);
    // If running and auto-saving => cannot change file name
    ui->outputDir->setDisabled(disableChangingFile);
    ui->saveFileFormat->setDisabled(disableChangingFile);
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

void MainWindow::saveSettings() {
    Settings settings;
    settings.setSamplingFrequency(ui->samplingFreq->currentText().toInt());
    settings.setFilterFrequency(ui->filterFreqSlider->value());
    settings.setOutputDirectry(ui->outputDir->text());
    settings.setFileNameFormat(ui->saveFileFormat->text());

    settings.setPortName(Settings::PortADC, ui->portChooser->currentText());
    settings.setPortSettings(Settings::PortADC, portSettingsADC);
    settings.setPortName(Settings::PortGPS, ui->portChooserGPS->currentText());
    settings.setPortSettings(Settings::PortGPS, portSettingsGPS);

    settings.setTableShown(ui->actionShowTable->isChecked());
    settings.setSettingsShown(ui->actionShowSettings->isChecked());
    settings.setStatsShown(ui->actionShowStats->isChecked());
    settings.setPlotFixedScale(ui->fixedScale->isChecked());
    settings.setPlotFixedScaleMax(ui->fixedScaleMax->value());
    settings.setPlotFixedScaleMin(ui->fixedScaleMin->value());
    settings.setPlotHistorySecs(ui->timeInterval->value());
}

bool MainWindow::askForClosing() {
    QMessageBox::StandardButton answer =
            QMessageBox::question(
                this, tr("Quit while receiving data?"),
                tr("Are you sure you want to quit?\nLast pieces of data may be lost, if they are not written on disk yet\n\nQuit and interrupt receiving of data?"),
                QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No),
                QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void MainWindow::closeEvent(QCloseEvent *e) {
    if (workerStarted && (false == askForClosing()) ) {
        e->ignore();
    } else {
        e->accept();
        // Calls Worker::finish and FileWriter::closeIfOpened
        emit finishing();
    }
}

MainWindow::~MainWindow()
{
    clockTimer->stop();

    saveSettings();

    threadFileWriter->quit();
    threadWorker->quit();
    if (false == threadWorker->wait(MAX_WAIT)) {
        Logger::error(tr("Worker thread hangs"));
    } else {
        delete worker;
    }
    if (false == threadFileWriter->wait(MAX_WAIT)) {
        Logger::error(tr("FileWriter thread hangs"));
    } else {
        delete fileWriter;
    }

    perfPlotting.reportResults();
    FileWriter::perfReporter.reportResults();
    perfStats.reportResults();
    perfDataView.reportResults();
    perfTotal.reportResults();
    SerialProtocol::generateTimestampsPerfReporter.reportResults();
    SerialProtocol::perfReporter.reportResults();
    TestProtocol::perfReporter.reportResults();
    perfTotal.flushDebug();

    delete ui;
}
