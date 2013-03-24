#include "worker.h"
#include "logger.h"

Worker::Worker(Protocol * prot, QObject *parent) :
    QObject(parent), protocol_(NULL)
{
    // NB: bool instance variables are initialized in reset (in finish)
    reset(prot);
}

void Worker::reset(Protocol *prot) {
    finish();
    protocol_ = prot;
    if(prot != NULL) {
        // Take ownership on protocol in order to prevent it from being deleted before Worker (and cause crash in destructor)
        protocol_->setParent(this);
    }
}

void Worker::prepare(bool autostart) {
    Q_ASSERT_X(protocol_ != 0, "Worker::prepare", "protocol not set");
    if( prepared ) {
        Logger::error(tr("Called prepare twice!"));
        setPrepared(PrepareAlready);
        return;
    }

    this->autostart = autostart;
    Logger::trace(tr("Opening protocol: %1...").arg(protocol_->description()));
    if(protocol_->open()) {
        Logger::info(tr("Opened protocol: %1").arg(protocol_->description()));

        // Connect signals before starting
        connect(protocol_, &Protocol::checkedADC, this, &Worker::onCheckedADC);
        connect(protocol_, &Protocol::checkedGPS, this, &Worker::onCheckedGPS);

        // Start checking
        Logger::trace(tr("Checking ADC..."));
        protocol_->checkADC();
    } else {
        Logger::error(tr("Failed to open protocol: %1").arg(protocol_->description()));
        setPrepared(PrepareFail);
    }
}

void Worker::onCheckedADC(bool success) {
    if(success) {
        Logger::info(tr("ADC ready"));
        if(protocol_->hasState(Protocol::GPSReady)) {
            // ADC checked, GPS ready => prepared!
            setPrepared(PrepareSuccess);
        } else {
            // Otherwise check it
            Logger::trace(tr("Checking GPS..."));
            protocol_->checkGPS();
        }
    } else {
        Logger::error(tr("ADC check failed!"));
        setPrepared(PrepareFail);
    }
}

void Worker::onCheckedGPS(bool success) {
    if(success) {
        Logger::info(tr("GPS ready"));
        if(protocol_->hasState(Protocol::ADCReady)) {
            // GPS checked, ADC ready => prepared!
            setPrepared(PrepareSuccess);
        } else {
            // Otherwise check it
            Logger::trace(tr("Checking ADC..."));
            protocol_->checkADC();
        }
    } else {
        Logger::error(tr("GPS check failed!"));
        setPrepared(PrepareFail);
    }
}

void Worker::setPrepared(PrepareResult res) {
    prepared = (res == PrepareSuccess);
    emit prepareFinished(res);

    if(prepared) {
        if(autostart) {
            Logger::warning(tr("Worker prepared and automatically started!"));
            start();
        } else {
            Logger::info(tr("Worker prepared and now can be started")) ;
        }
    }
}

Worker::StartResult Worker::start() {
    Q_ASSERT_X(protocol_ != 0, "Worker::start", "protocol not set");
    if( ! prepared ) {
        Logger::error(tr("Trying to start not prepared worker!"));
        return StartFailNotPrepared;
    }
    if( started ) {
        Logger::error(tr("Trying to start already started worker!"));
        return StartFailAlreadyStarted;
    }

    started = true;
    connect(protocol_, &Protocol::dataAvailable, this, &Worker::onDataAvailable);

    Logger::trace(tr("Starting receiving data..."));
    protocol_->startReceiving();

    return StartSuccess;
}

void Worker::onDataAvailable(DataVector newData) {
    Logger::trace(tr("Received %1 data items").arg(newData.size()));
    data_ += newData;
    /// TODO: Writing into file here? How often?
    emit dataUpdated(newData);
}

void Worker::pause() {
    Q_ASSERT_X(protocol_ != 0, "Worker::pause", "protocol not set");
    if( paused ) {
        return;
    }
    paused = true;
    Logger::warning(tr("Paused receiving data!"));
    protocol_->stopReceiving();
}

void Worker::unpause() {
    Q_ASSERT_X(protocol_ != 0, "Worker::unpause", "protocol not set");
    if( ! paused ) {
        return;
    }
    paused = false;
    Logger::info(tr("Continuing receiving data after pause..."));
    protocol_->startReceiving();
}

void Worker::finish() {
    if(protocol_ != NULL) {
        if(protocol_->hasState(Protocol::Open)) {
            protocol_->close();
            Logger::info(tr("Closed protocol: %1").arg(protocol_->description()));
        }
        protocol_->disconnect(this);
    }
    autostart = false;
    prepared = false;
    started = false;
    paused = false;
}
