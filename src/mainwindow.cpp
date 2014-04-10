#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QFileDialog>
#include <QList>
#include <QElapsedTimer>

#include "protocols/testprotocol.h"
#include "protocols/serialprotocol.h"
#include "logger.h"
#include "settings.h"
#include "gui/statsbox.h"
#include "gui/timeplot.h"
#include "gui/portsettingsdialog.h"

namespace {
    const QString TEST_PROTOCOL = "TEST";
    const int FREQ_200 = 200;
    const int FREQ_50  = 50;
    const int FREQ_1   = 1;

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
    Protocol * makeProtocol(QString portName, int samplingFrequency, int filterFrequency, QObject * parent, PortSettingsEx portSettings = SerialProtocol::DEFAULT_PORT_SETTINGS) {
        if(portName == TEST_PROTOCOL) {
            // An option for testing
            return new TestProtocol(samplingFrequency, 9000000, parent);
        } else {
            return new SerialProtocol(portName, samplingFrequency, filterFrequency, portSettings, parent);
        }
    }

}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), protocolADC(NULL), protocolGPS(NULL), receivedItems(0),
    perfPlotting(tr("Plotting")), perfWritting(tr("Writing")), perfTotal(tr("Total"))
{
    ui->setupUi(this);
    worker = new Worker(NULL, NULL, this);
    fileWriter = new FileWriter(FileWriter::DEFAULT_FILENAME_PREFIX, FileWriter::DEFAULT_FILENAME_SUFFIX, this);

    initWidgetsArray(plots, ui->plotArea, ui->plotArea2, ui->plotArea3);
    initWidgetsArray(stats, ui->stats, ui->stats2, ui->stats3);

    setup();
}

void MainWindow::setup() {
    Settings settings;
    // Init GUI
    initPortChooser(ui->portChooser, settings.portName(Settings::PortADC));
    initPortChooser(ui->portChooserGPS, settings.portName(Settings::PortGPS));
    initFreqChooser(ui->samplingFreq, QList<int>({FREQ_200, FREQ_50, FREQ_1}), settings.samplingFrequency());
    initFreqChooser(ui->filterFreq,   QList<int>({FREQ_200, FREQ_50}),         settings.filterFrequency());
    fileWriter->setFileName(settings.fileNamePrefix(), settings.fileNameSuffix());
    fileWriter->setDeviceID(settings.deviceId());
    initFileHandlers();
    disableOnConnect << ui->portChooser << ui->portChooserGPS
                     << ui->samplingFreq << ui->filterFreq
                     << ui->portSettingsADC << ui->portSettingsGPS;

    initShowHideAction(ui->actionShowTable,    ui->dataView, settings.isTableShown());
    initShowHideAction(ui->actionShowSettings, ui->settings, settings.isSettingsShown());
    for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
        initShowHideAction(ui->actionShowStats, stats[ch], settings.isStatsShown());

        plots[ch]->setChannel(ch);
    }
    ui->connectBtn->setFocus();
    // Plot settings
    void (QSpinBox:: *valueChangedSignal)(int) = &QSpinBox::valueChanged; // resolve overloaded function
    connect(ui->fixedScaleMax, valueChangedSignal, [=](){ui->fixedScale->setChecked(true);});
    for (TimePlot *plot:  plots) {
        connect(ui->fixedScale,    &QAbstractButton::toggled, plot, &TimePlot::setFixedScaleY);
        connect(ui->fixedScaleMax, valueChangedSignal,        plot, &TimePlot::setFixedScaleYMax);
        connect(ui->timeInterval,  valueChangedSignal,        plot, &TimePlot::setHistorySecs);

        plot->setFixedScaleYMax(settings.plotFixedScaleMax());
        plot->setFixedScaleY(settings.isPlotFixedScale());
        plot->setHistorySecs(settings.plotHistorySecs());
    }
    (settings.isPlotFixedScale() ? ui->fixedScale : ui->autoScale)->setChecked(true);
    ui->fixedScaleMax->setValue( settings.plotFixedScaleMax() );
    ui->timeInterval->setValue(  settings.plotHistorySecs()   );

    portSettingsADC = settings.portSettigns(Settings::PortADC);
    portSettingsGPS = settings.portSettigns(Settings::PortGPS);
    initPortSettingsAction(ui->actionADCPortSettings, ui->actionADCPortSettings->text(), portSettingsADC, ui->portSettingsADC);
    initPortSettingsAction(ui->actionGPSPortSettings, ui->actionGPSPortSettings->text(), portSettingsGPS, ui->portSettingsGPS);

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
        foreach(QWidget * w, disableOnConnect) {
            w->setDisabled(true);
        }

        int samplingFrequency = ui->samplingFreq->currentText().toInt();
        int filterFrequency   = ui->filterFreq->currentText().toInt();
        for(TimePlot * plot: plots) {
            plot->setPointsPerSec(samplingFrequency);
        }
        fileWriter->setFrequencies(samplingFrequency, filterFrequency);

        QString portNameADC = ui->portChooser->currentText();
        QString portNameGPS = ui->portChooserGPS->currentText();
        protocolADC = makeProtocol(portNameADC, samplingFrequency, filterFrequency, this, portSettingsADC);
        if (portNameADC == portNameGPS) {
            // Important! If port names are equal, protocols also should be the
            // same instance, NOT two different instances with the same parameters!
            protocolGPS = protocolADC;
            // TODO: move this `if` into makeProtocol and move this function to core?
        } else {
            protocolGPS = makeProtocol(portNameGPS, samplingFrequency, filterFrequency, this, portSettingsGPS);
        }

        worker->reset(protocolADC, protocolGPS);
        initWorkerHandlers();
        worker->prepare();
    });
    connect(ui->startBtn, &QPushButton::clicked, [=](){
        ui->startBtn->setDisabled(true);
        ui->stopBtn->setEnabled(true);
        ui->stopBtn->setFocus();

        startedAt = QDateTime::currentDateTime();
        ui->timeStart->setDateTime(startedAt);
        ui->timeElapsed->setTime(QTime(0,0,0));

        // TODO: do not call Worker and FileWriter methods, send signals instead
        // (for future parallel implementation)
        if(worker->isStarted()) {
            worker->unpause();
        } else {
            worker->start();
            setFileControlsState();
        }
    });
    connect(ui->stopBtn, &QPushButton::clicked, [=](){
        ui->stopBtn->setDisabled(true);
        ui->startBtn->setEnabled(true);
        ui->startBtn->setFocus();

        worker->pause();
        setFileControlsState();
    });
    connect(ui->disconnectBtn, &QPushButton::clicked, [=](){
        worker->finish();
        for(TimePlot * plot: plots) {
            plot->clearHistory();
        }
        setReceivedItems(0);
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
    connect(ui->disconnectBtn, &QPushButton::clicked, fileWriter, &FileWriter::finishFile);
}

void MainWindow::initWorkerHandlers() {
    worker->disconnect();
    connect(worker->protocolADC(), &Protocol::checkedADC, [=](bool success){
        ui->ledADC->setOnColor( success ? QLed::Green : QLed::Red);
        ui->ledADC->setValue(true);
    });
    connect(worker->protocolGPS(), &Protocol::checkedGPS, [=](bool success){
        ui->ledGPS->setOnColor( success ? QLed::Green : QLed::Red);
        ui->ledGPS->setValue(true);
    });
    connect(worker->protocolGPS(), &Protocol::timeAvailable, [=](QDateTime time){
        Logger::info(tr("Received time update: %1UTC").arg(time.toString("yyyy-MM-dd hh:mm:ss.zzz")));
        // TODO: set as system time
    });
    connect(worker->protocolGPS(), &Protocol::positionAvailable, [=](double latitude, double longitude, double altitude){
        Logger::info(tr("Received position update: %1, %2, %3m").arg(latitude).arg(longitude).arg(altitude));
        // TODO: convert to minutes/seconds format?
        QString latitudeStr  = QString::number(latitude);
        QString longitudeStr = QString::number(longitude);

        ui->currentLatitude->setText(latitudeStr);
        ui->currentLongitude->setText(longitudeStr);
        fileWriter->setCoordinates(latitudeStr, longitudeStr);
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
            ui->connectBtn->setFocus();
        }
    });
    connect(worker, &Worker::dataUpdated, this, &MainWindow::onDataReceived);
}

void MainWindow::initFileHandlers() {

    connect(this,               &MainWindow::autoWriteChanged, fileWriter, &FileWriter::setAutoWriteEnabled);
    connect(ui->saveFilePrefix, &QLineEdit::editingFinished,   this,       &MainWindow::onFileNameChanged);
    connect(ui->saveFileSuffix, &QLineEdit::editingFinished,       this,       &MainWindow::onFileNameChanged);
    connect(ui->writeNowBtn,    &QPushButton::clicked,         fileWriter, &FileWriter::writeOnce);
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

    /* TODO: return Browse button!
     connect(ui->browseBtn, &QPushButton::clicked, [=](){
        QString file = QFileDialog::getSaveFileName(this, tr("Choose file for writing data"), ui->saveFileName->text());
        if( ! file.isEmpty()) {
            ui->saveFileName->setText(file);
            // this will automatically notify fileWriter: changing ui->saveFileName text
            // will trigger textChanged, which is connected to setFileName of FileWriter (see above)
        }
    });*/

    // Set initial values
    ui->saveFilePrefix->setText(fileWriter->fileNamePrefix());
    ui->saveFileSuffix->setText(fileWriter->fileNameSuffix());
    emit autoWriteChanged(ui->writeToFileEnabled->isChecked());
    setFileControlsState();
}

void MainWindow::onDataReceived(TimeStampsVector t, DataVector d) {
    QElapsedTimer timerTotal; timerTotal.start();

    Logger::trace(tr("Received %1 data items").arg(d.size()*CHANNELS_NUM));

    setReceivedItems(receivedItems + d.size()*CHANNELS_NUM);
//        TODO: dataView is currently disabled! Find a way to enable it without lags
//        QStringList items;
//        foreach(DataItem item, d) {
//            // TODO: use table instead of list
//            QStringList itemStr;
//            for(unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
//                itemStr << QString::number(item.byChannel[ch]);
//            }
//            items << itemStr.join("; ");
//        }
//        ui->dataView->addItems(items);
//        ui->dataView->scrollToBottom();
    for (unsigned ch = 0; ch < CHANNELS_NUM; ++ch) {
        // Update stats
        stats[ch]->setStats(d, ch);
    }

    // TODO: don't call these slots (FileWriter::receiveData and TimePlot::receiveData), connect them separately.
    // Currently it is like that for performance measurements.
    QElapsedTimer timerWriting; timerWriting.start();
    fileWriter->receiveData(t, d);
    perfWritting.addMeasurement(timerWriting.elapsed());

    QElapsedTimer timerPlotting; timerPlotting.start();
    for(TimePlot * plot: plots) {
        plot->receiveData(t, d);
    }
    perfPlotting.addMeasurement(timerPlotting.elapsed());

    perfTotal.addMeasurement(timerTotal.elapsed());
}

void MainWindow::setReceivedItems(int received) {
    receivedItems = received;
    ui->samplesRcvd->setText(QString::number(receivedItems));
}

void MainWindow::onFileNameChanged() {
    fileWriter->setFileName(ui->saveFilePrefix->text(), ui->saveFileSuffix->text());
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

void MainWindow::setFileControlsState() {
    bool disableChangingFile = (ui->writeToFileEnabled->isChecked() && worker->isStarted());
    // If running and auto-saving => cannot change file name
    ui->saveFilePrefix->setDisabled(disableChangingFile);
    ui->saveFileSuffix->setDisabled(disableChangingFile);
    /* TODO: return Browse button
    ui->browseBtn->setDisabled(disableChangingFile); */

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
    settings.setFilterFrequency(ui->filterFreq->currentText().toInt());
    settings.setFileNamePrefix(ui->saveFilePrefix->text());
    settings.setFileNameSuffix(ui->saveFileSuffix->text());

    settings.setPortName(Settings::PortADC, ui->portChooser->currentText());
    settings.setPortSettings(Settings::PortADC, portSettingsADC);
    settings.setPortName(Settings::PortGPS, ui->portChooserGPS->currentText());
    settings.setPortSettings(Settings::PortGPS, portSettingsGPS);

    settings.setTableShown(ui->actionShowTable->isChecked());
    settings.setSettingsShown(ui->actionShowSettings->isChecked());
    settings.setStatsShown(ui->actionShowStats->isChecked());
    settings.setPlotFixedScale(ui->fixedScale->isChecked());
    settings.setPlotFixedScaleMax(ui->fixedScaleMax->value());
    settings.setPlotHistorySecs(ui->timeInterval->value());
}

MainWindow::~MainWindow()
{
    clockTimer->stop();

    perfPlotting.reportResults();
    perfWritting.reportResults();
    perfTotal.reportResults();
    perfTotal.flushDebug();

    saveSettings();
    delete worker;
    delete fileWriter;
    delete ui;
}
