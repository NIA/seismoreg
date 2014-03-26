#include "portsettingsdialog.h"
#include "ui_portsettingsdialog.h"
#include <QVector>

#ifdef Q_OS_WIN
#define _ONLY_WIN32(item) item,
#else
#define _ONLY_WIN32(_)
#endif // Q_OS_WIN

Q_DECLARE_METATYPE(BaudRateType)
Q_DECLARE_METATYPE(DataBitsType)
Q_DECLARE_METATYPE(StopBitsType)
Q_DECLARE_METATYPE(ParityType)
Q_DECLARE_METATYPE(FlowType)

PortSettingsDialog::PortSettingsDialog(QString title, PortSettingsEx portSettings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PortSettingsDialog),
    settings(portSettings)
{
    ui->setupUi(this);
    setWindowTitle(title);
    setInputValues();
}

void PortSettingsDialog::accept() {
    getInputValues();
    QDialog::accept();
}

PortSettingsDialog::~PortSettingsDialog() {
    delete ui;
}

namespace {
    const QVector<BaudRateType> baudRateValues = {
        BAUD110, BAUD300, BAUD600, BAUD1200, BAUD2400, BAUD4800, BAUD9600,
        _ONLY_WIN32(BAUD14400) BAUD19200, BAUD38400, _ONLY_WIN32(BAUD56000)
        BAUD57600, BAUD115200, _ONLY_WIN32(BAUD128000) _ONLY_WIN32(BAUD256000)
    };
    const QVector<DataBitsType> dataBitsValues = { DATA_5, DATA_6, DATA_7, DATA_8 };
    const QVector<StopBitsType> stopBitsValues = { STOP_1, _ONLY_WIN32(STOP_1_5) STOP_2 };
    const QVector<ParityType>   parityValues   = { PAR_NONE, PAR_ODD, PAR_EVEN, _ONLY_WIN32(PAR_MARK) PAR_SPACE };
    const QVector<FlowType>     flowValues     = { FLOW_OFF, FLOW_HARDWARE, FLOW_XONXOFF };

    QString toString(BaudRateType value) { return QString::number(value); }
    QString toString(DataBitsType value) { return QString::number(value); }
    QString toString(StopBitsType value) {
        switch (value) {
        case STOP_1:   return "1";
        case STOP_1_5: return "1.5";
        case STOP_2:   return "2";
        default:       return "";
        }
    }
    QString toString(ParityType value) {
        switch (value) {
        case PAR_SPACE: return PortSettingsDialog::tr("Space");
        case PAR_MARK:  return PortSettingsDialog::tr("Mark");
        case PAR_NONE:  return PortSettingsDialog::tr("None");
        case PAR_EVEN:  return PortSettingsDialog::tr("Even");
        case PAR_ODD:   return PortSettingsDialog::tr("Odd");
        default:        return "";
        }
    }
    QString toString(FlowType value) {
        switch (value) {
        case FLOW_OFF:      return PortSettingsDialog::tr("None");
        case FLOW_HARDWARE: return PortSettingsDialog::tr("Hardware");
        case FLOW_XONXOFF:  return PortSettingsDialog::tr("Software");
        default:            return "";
        }
    }

    template <typename T>
    void initChooser(QComboBox *chooser, QVector<T> values, T initialValue) {
        foreach(T value, values) {
            chooser->addItem(toString(value), QVariant::fromValue(value));
            if (initialValue == value) {
                // select this (last) value
                chooser->setCurrentIndex(chooser->count() - 1);
            }
        }
    }

    template <typename T>
    void getValue(QComboBox *chooser, T & value) {
        value = chooser->itemData( chooser->currentIndex() ).value<T>();
    }
}

void PortSettingsDialog::setInputValues() {
    initChooser(ui->baudRate,    baudRateValues, settings.BaudRate);
    initChooser(ui->dataBits,    dataBitsValues, settings.DataBits);
    initChooser(ui->stopBits,    stopBitsValues, settings.StopBits);
    initChooser(ui->parity,      parityValues,   settings.Parity);
    initChooser(ui->flowControl, flowValues,     settings.FlowControl);
    // TODO: suppot timeout setting?
    ui->debug->setChecked(settings.debug);
}

void PortSettingsDialog::getInputValues() {
    getValue(ui->baudRate, settings.BaudRate);
    getValue(ui->dataBits, settings.DataBits);
    getValue(ui->stopBits, settings.StopBits);
    getValue(ui->parity,   settings.Parity);
    getValue(ui->flowControl, settings.FlowControl);
    settings.debug = ui->debug->isChecked();
}
