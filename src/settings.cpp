#include "settings.h"
#include <QHash>
#include <QDir>

#include "gui/timeplot.h" // for timeplot defaults
#include "filewriter.h"   // for filewriter defaults

namespace {
    const QString SETTINGS_FILE = "seismoreg.ini";
    const QString DEVICE_ID_FILE_DEFAULT = "C:/Users/NIA/Desktop/config.ini";

    /**
     * NB: keys are case-insensitive in INI format,
     * that's why it's better to have them all in
     * the same case to avoid collision
     */

    // Prefixes
    const QString CORE_PREFIX = "core/";
    const QString ADC_PORT_PREFIX  = "port_adc/";
    const QString GPS_PORT_PREFIX  = "port_gps/";
    const QString GUI_PREFIX  = "gui/";
    const QString LOG_PREFIX  = "log/";

    QString prefixFor(Settings::WhichPort port) {
        return (port == Settings::PortADC) ? ADC_PORT_PREFIX : GPS_PORT_PREFIX;
    }

    // Complete keys
    const QString DEVICE_ID  = CORE_PREFIX + "device_id";
    const QString SAMPL_FREQ = CORE_PREFIX + "sampling_frequency";
    const QString FILTR_FREQ = CORE_PREFIX + "filter_frequency";
    const QString OUTPUT_DIR = CORE_PREFIX + "output_dir";
    const QString FILE_FORMAT= CORE_PREFIX + "filename_format";
    const QString TABLE_SHOWN    = GUI_PREFIX + "table_shown";
    const QString SETTINGS_SHOWN = GUI_PREFIX + "settings_shown";
    const QString STATS_SHOWN    = GUI_PREFIX + "stats_shown";
    const QString FIXED_SCALE    = GUI_PREFIX + "fixed_scale";
    const QString FIXED_SCALE_MAX= GUI_PREFIX + "fixed_scale_max";
    const QString FIXED_SCALE_MIN= GUI_PREFIX + "fixed_scale_min";
    const QString HISTORY_SECS   = GUI_PREFIX + "history_secs";
    // Parameterized key: use with .arg(levelToStr[level])
    const QString LOG_LEV_ENABLED= LOG_PREFIX + "%1_enabled";
    // Incomplete keys (should be combined with either ADC_PORT_PREFIX or GPS_PORT_PREFIX)
    const QString _PORT =  "port";
    const QString _BAUD_RATE = "baud_rate";
    const QString _DATA_BITS = "data_bits";
    const QString _STOP_BITS = "stop_bits";
    const QString _PARITY    = "parity";
    const QString _FLOW_CONTROL = "flow_control";
    const QString _TIMEOUT   = "timeout";
    const QString _DEBUG_MODE= "debug";

    // Default values
    const int DEVICE_ID_DEFAULT = 1;
    const QString PORT_DEFAULT  = "TEST";
    const int FREQUENCY_DEFAULT = 200;
    const bool TABLE_SHOWN_DEFAULT    = false;
    const bool SETTINGS_SHOWN_DEFAULT = true;
    const bool STATS_SHOWN_DEFAULT    = true;
    const bool LOG_LEV_ENABLED_DEFAULT= true;

    const QString levelToStr[Logger::_levelsCount] = {"trace", "info", "warning", "error"};

    template<class T>
    QHash<T,QString> stringMap();
    template<>
    QHash<StopBitsType,QString> stringMap<StopBitsType>() {
        QHash<StopBitsType,QString> res;
        res[STOP_1] = "1";
#if defined(Q_OS_WIN)
        res[STOP_1_5] = "1.5";
#endif
        res[STOP_2] = "2";
        return res;
    }
    template<>
    QHash<ParityType,QString> stringMap<ParityType>() {
        QHash<ParityType,QString> res;
        res[PAR_NONE]  = "none";
        res[PAR_ODD]   = "odd";
        res[PAR_EVEN]  = "even";
#if defined(Q_OS_WIN)
        res[PAR_MARK]  = "mark";
#endif
        res[PAR_SPACE] = "space";
        return res;
    }
    template<>
    QHash<FlowType,QString> stringMap<FlowType>() {
        QHash<FlowType,QString> res;
        res[FLOW_OFF]      = "none";
        res[FLOW_HARDWARE] = "hardware";
        res[FLOW_XONXOFF]  = "software";
        return res;
    }

    template<typename T>
    QVariant toStrVariant(T value) {
        return stringMap<T>()[value];
    }
    template<typename T>
    T fromStrVariant(QVariant variant, T defaultValue) {
        QString val = variant.toString().trimmed();
        QHash<T,QString> map = stringMap<T>();
        if (map.values().contains(val)) {
            return map.key(val);
        } else {
            return defaultValue;
        }
    }

    template<typename T>
    QVariant toIntVariant(T value) {
        return QVariant(static_cast<int>(value));
    }
    template<typename T>
    T fromIntVariant(QVariant variant, T defaultValue) {
        bool ok;
        int res = variant.toInt(&ok);
        // TODO: check if value is correct enum item?
        return ok ? static_cast<T>(res) : defaultValue;
    }

}

Settings::Settings(QObject *parent) :
    QObject(parent), settings(SETTINGS_FILE, QSettings::IniFormat, this)
{
}

// Core settings

QString Settings::deviceId() const {
    // First try to get it from external file
    QFile file(DEVICE_ID_FILE_DEFAULT); // TODO: get path from settings too
    if( file.open(QFile::ReadOnly | QFile::Text) ) {
        QTextStream in(&file);
        QString line = in.readLine().trimmed().toLower();
        // first line of file should have format [device ID]=02
        if (line.startsWith("[device id]")) {
            int pos = line.indexOf('=');
            QString id = line.mid(pos + 1);
            return id;
        }
        file.close();
    }
    return settings.value(DEVICE_ID, DEVICE_ID_DEFAULT).toString();
}

void Settings::setDeviceId(const QString &value) {
    settings.setValue(DEVICE_ID, value);
}

int Settings::samplingFrequency() const {
    return settings.value(SAMPL_FREQ, FREQUENCY_DEFAULT).toInt();
}
void Settings::setSamplingFrequency(int value) {
    settings.setValue(SAMPL_FREQ, value);
}

int Settings::filterFrequency() const {
    return settings.value(FILTR_FREQ, FREQUENCY_DEFAULT).toInt();
}
void Settings::setFilterFrequency(int value) {
    settings.setValue(FILTR_FREQ, value);
}

QString Settings::outputDirectory() const {
    QString dir = settings.value(OUTPUT_DIR, FileWriter::DEFAULT_OUTPUT_DIR).toString();
    if (dir == ".") {
        return QDir::currentPath();
    } else {
        return dir;
    }
}
void Settings::setOutputDirectry(const QString &value) {
    QString dir = (value == QDir::currentPath()) ? "." : value;
    settings.setValue(OUTPUT_DIR, dir);
}

QString Settings::fileNameFormat() const {
    return settings.value(FILE_FORMAT, FileWriter::DEFAULT_FILENAME_FORMAT).toString();
}
void Settings::setFileNameFormat(const QString &value) {
    settings.setValue(FILE_FORMAT, value);
}

// Ports settings

QString Settings::portName(Settings::WhichPort port) const {
    return settings.value(prefixFor(port) + _PORT, PORT_DEFAULT).toString();
}
void Settings::setPortName(Settings::WhichPort port, const QString &value) {
    settings.setValue(prefixFor(port) + _PORT, value);
}

BaudRateType Settings::baudRate(Settings::WhichPort port) const {
    return fromIntVariant<BaudRateType>( settings.value(prefixFor(port) + _BAUD_RATE),
                                         SerialProtocol::DEFAULT_PORT_SETTINGS.BaudRate );
}
void Settings::setBaudRate(Settings::WhichPort port, BaudRateType value) {
    settings.setValue(prefixFor(port) + _BAUD_RATE, toIntVariant(value));
}

DataBitsType Settings::dataBits(Settings::WhichPort port) const {
    return fromIntVariant<DataBitsType>( settings.value(prefixFor(port) + _DATA_BITS),
                                         SerialProtocol::DEFAULT_PORT_SETTINGS.DataBits );
}
void Settings::setDataBits(Settings::WhichPort port, DataBitsType value) {
    settings.setValue(prefixFor(port) + _DATA_BITS, toIntVariant(value));
}

StopBitsType Settings::stopBits(Settings::WhichPort port) const {
    return fromStrVariant<StopBitsType>( settings.value(prefixFor(port) + _STOP_BITS),
                                         SerialProtocol::DEFAULT_PORT_SETTINGS.StopBits );
}
void Settings::setStopBits(Settings::WhichPort port, StopBitsType value) {
    settings.setValue(prefixFor(port) + _STOP_BITS, toStrVariant(value));
}

ParityType Settings::parity(Settings::WhichPort port) const {
    return fromStrVariant<ParityType>( settings.value(prefixFor(port) + _PARITY),
                                       SerialProtocol::DEFAULT_PORT_SETTINGS.Parity );
}
void Settings::setParity(Settings::WhichPort port, ParityType value) {
    settings.setValue(prefixFor(port) + _PARITY, toStrVariant(value));
}

FlowType Settings::flowControl(Settings::WhichPort port) const {
    return fromStrVariant<FlowType>( settings.value(prefixFor(port) + _FLOW_CONTROL),
                                     SerialProtocol::DEFAULT_PORT_SETTINGS.FlowControl);
}
void Settings::setFlowControl(Settings::WhichPort port, FlowType value) {
    settings.setValue(prefixFor(port) + _FLOW_CONTROL, toStrVariant(value));
}

bool Settings::debugMode(Settings::WhichPort port) const {
    return settings.value(prefixFor(port) + _DEBUG_MODE,
                          SerialProtocol::DEFAULT_PORT_SETTINGS.debug).toBool();
}
void Settings::setDebugMode(Settings::WhichPort port, bool value) {
    settings.setValue(prefixFor(port) + _DEBUG_MODE, value);
}

PortSettingsEx Settings::portSettigns(Settings::WhichPort port) const {
    PortSettingsEx result = SerialProtocol::DEFAULT_PORT_SETTINGS;
    result.BaudRate = baudRate(port);
    result.DataBits = dataBits(port);
    result.StopBits = stopBits(port);
    result.Parity   = parity(port);
    result.FlowControl = flowControl(port);
    result.debug    = debugMode(port);
    // TODO: add timeout setting?
    return result;
}
void Settings::setPortSettings(Settings::WhichPort port, PortSettingsEx value) {
    setBaudRate(port, value.BaudRate);
    setDataBits(port, value.DataBits);
    setStopBits(port, value.StopBits);
    setParity  (port, value.Parity);
    setFlowControl(port, value.FlowControl);
    setDebugMode  (port, value.debug);
    // TODO: add timeout setting?
}

// Log settings

bool Settings::isLevelEnabled(Logger::Level level) const {
    return settings.value(LOG_LEV_ENABLED.arg(levelToStr[level]),
                          LOG_LEV_ENABLED_DEFAULT).toBool();
}
void Settings::setLevelEnabled(Logger::Level level, bool value) {
    settings.setValue(LOG_LEV_ENABLED.arg(levelToStr[level]), value);
}

// GUI settings

bool Settings::isTableShown() const {
    return settings.value(TABLE_SHOWN, TABLE_SHOWN_DEFAULT).toBool();
}
void Settings::setTableShown(bool value) {
    settings.setValue(TABLE_SHOWN, value);
}

bool Settings::isSettingsShown() const {
    return settings.value(SETTINGS_SHOWN, SETTINGS_SHOWN_DEFAULT).toBool();
}
void Settings::setSettingsShown(bool value) {
    settings.setValue(SETTINGS_SHOWN, value);
}

bool Settings::isStatsShown() const {
    return settings.value(STATS_SHOWN, STATS_SHOWN_DEFAULT).toBool();
}
void Settings::setStatsShown(bool value) {
    settings.setValue(STATS_SHOWN, value);
}

bool Settings::isPlotFixedScale() const {
    return settings.value(FIXED_SCALE, TimePlot::FIXED_SCALE_DEFAULT).toBool();
}
void Settings::setPlotFixedScale(bool value) {
    settings.setValue(FIXED_SCALE, value);
}

int Settings::plotFixedScaleMax() const {
    return settings.value(FIXED_SCALE_MAX, TimePlot::FIXED_SCALE_MAX_DEFAULT).toInt();
}
void Settings::setPlotFixedScaleMax(int value) {
    settings.setValue(FIXED_SCALE_MAX, value);
}
int Settings::plotFixedScaleMin() const {
    return settings.value(FIXED_SCALE_MIN, TimePlot::FIXED_SCALE_MIN_DEFAULT).toInt();
}
void Settings::setPlotFixedScaleMin(int value) {
    settings.setValue(FIXED_SCALE_MIN, value);
}

int Settings::plotHistorySecs() const {
    return settings.value(HISTORY_SECS, TimePlot::HISTORY_SECONDS_DEFAULT).toInt();
}
void Settings::setPlotHistorySecs(int value) {
    settings.setValue(HISTORY_SECS, value);
}
