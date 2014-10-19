#include "worker.h"
#include "logger.h"

Worker::Worker(QObject *parent)
    : QObject(parent), protocolADC_(NULL), protocolGPS_(NULL)
{
    // Set all params to initial values
    finish();
}

void Worker::reset(ProtocolCreator *protADC, ProtocolCreator *protGPS) {
    finish();
    assignProtocol(protocolADC_, protADC);
    assignProtocol(protocolGPS_, protGPS);
}

void Worker::assignProtocol(Protocol *&lvalue, ProtocolCreator *rvalue) {
    if (rvalue != NULL) {
        lvalue = rvalue->createProtocol();
        // Take ownership on protocol in order to prevent it from being deleted before Worker (and cause crash in destructor)
        lvalue->setParent(this);
    } else {
        lvalue = NULL;
    }
}

void Worker::prepare(bool autostart) {
    Q_ASSERT_X(protocolADC_ != 0, "Worker::prepare", "ADC protocol not set");
    Q_ASSERT_X(protocolGPS_ != 0, "Worker::prepare", "GPS protocol not set");
    if ( prepared ) {
        Logger::error(tr("Called prepare twice!"));
        setPrepared(PrepareAlready);
        return;
    }

    this->autostart = autostart;

    // Open first port:
    Logger::trace(tr("Opening protocol: %1...").arg(protocolADC_->description()));
    if (protocolADC_->open()) {
        Logger::info(tr("Opened protocol: %1").arg(protocolADC_->description()));

        // If OK, open second port (if it is different):
        if (protocolADC_ != protocolGPS_) {
            Logger::trace(tr("Opening GPS protocol: %1...").arg(protocolGPS_->description()));
            if (protocolGPS_->open()) {
                Logger::info(tr("Opened protocol: %1").arg(protocolGPS_->description()));
                // all ok, continue
            } else {
                Logger::error(tr("Failed to open GPS protocol: %1").arg(protocolGPS_->description()));
                setPrepared(PrepareFailGPS);
                // not ok, interrupt
                return;
            }
            // TODO: but what if they are different instances of SerialProtocol with same port value? This should somehow be prohibited.
        }

        // Transmit signals from protocols ("internal") by emiting new signals ("public")
        connect(protocolADC_, &Protocol::checkedADC, this, &Worker::checkedADC);
        connect(protocolGPS_, &Protocol::checkedGPS, this, &Worker::checkedGPS);
        connect(protocolGPS_, &Protocol::timeAvailable, this, &Worker::timeAvailable);
        connect(protocolGPS_, &Protocol::positionAvailable, this, &Worker::positionAvailable);
        // Connect signals before starting
        connect(protocolADC_, &Protocol::checkedADC, this, &Worker::onCheckedADC);
        connect(protocolGPS_, &Protocol::checkedGPS, this, &Worker::onCheckedGPS);

        // Start checking
        Logger::trace(tr("Checking ADC..."));
        protocolADC_->checkADC();
    } else {
        Logger::error(tr("Failed to open ADC protocol: %1").arg(protocolADC_->description()));
        setPrepared(PrepareFailADC);
    }
}

void Worker::onCheckedADC(bool success) {
    if (prepared) {
        // If already prepared, no need to notify anyone
        return;
    }
    if (success) {
        Logger::info(tr("ADC ready"));
        if(protocolGPS_->hasState(Protocol::GPSReady)) {
            // ADC checked, GPS ready => prepared!
            setPrepared(PrepareSuccess);
        } else {
            // Otherwise check it
            Logger::trace(tr("Checking GPS..."));
            protocolGPS_->checkGPS();
        }
    } else {
        Logger::error(tr("ADC check failed!"));
        setPrepared(PrepareFailADC);
    }
}

void Worker::onCheckedGPS(bool success) {
    if (prepared) {
        // If already prepared, no need to notify anyone
        return;
    }
    if (success) {
        Logger::info(tr("GPS ready"));
        if(protocolADC_->hasState(Protocol::ADCReady)) {
            // GPS checked, ADC ready => prepared!
            setPrepared(PrepareSuccess);
        } else {
            // Otherwise check it
            Logger::trace(tr("Checking ADC..."));
            protocolADC_->checkADC();
        }
    } else {
        Logger::error(tr("GPS check failed!"));
        setPrepared(PrepareFailGPS);
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

void Worker::start() {
    Q_ASSERT_X(protocolADC_ != 0, "Worker::start", "ADC protocol not set");
    if( ! prepared ) {
        Logger::error(tr("Trying to start not prepared worker!"));
        emit triedToStart(StartFailNotPrepared);
        return;
    }
    if( started ) {
        Logger::error(tr("Trying to start already started worker!"));
        emit triedToStart(StartFailAlreadyStarted);
        return;
    }

    started = true;
    connect(protocolADC_, &Protocol::dataAvailable, this, &Worker::dataUpdated, Qt::UniqueConnection);

    Logger::trace(tr("Starting receiving data..."));
    protocolADC_->startReceiving();

    emit triedToStart(StartSuccess);
}

void Worker::stop() {
    Logger::info(tr("Receiving data stopped"));
    protocolADC_->stopReceiving();
    started = false;
    emit stopped();
}

void Worker::finalizeProtocol(Protocol * protocol) {
    if(protocol != NULL) {
        if(protocol->hasState(Protocol::Open)) {
            protocol->close();
            Logger::info(tr("Closed protocol: %1").arg(protocol->description()));
        }
        protocol->disconnect(this);
    }
}

void Worker::finish() {
    finalizeProtocol(protocolADC_);
    if (protocolADC_ != protocolGPS_) {
        finalizeProtocol(protocolGPS_);
    }
    autostart = false;
    prepared = false;
    started = false;
    emit finished();
}

void Worker::setFrequencies(int samplingFreq, int filterFreq)
{
    Q_ASSERT_X(protocolADC_ != 0, "Worker::setFrequencies", "protocol not set");
    if ( ! started ) {
        protocolADC_->setSamplingFrequency(samplingFreq);
        protocolADC_->setFilterFrequency(filterFreq);
    } else {
        Logger::error("Cannot change frequencies when started");
    }
}

