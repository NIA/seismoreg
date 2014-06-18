#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include "protocols/serialprotocol.h"
#include "logger.h"

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = nullptr);

    // Core settings

    int deviceId() const;
    void setDeviceId(int value);

    int samplingFrequency() const;
    void setSamplingFrequency(int value);

    int filterFrequency() const;
    void setFilterFrequency(int value);

    QString outputDirectory() const;
    void setOutputDirectry(const QString &value);

    QString fileNameFormat() const;
    void setFileNameFormat(const QString &value);

    // Ports settings

    enum WhichPort {
        PortADC,
        PortGPS
    };

    QString portName(WhichPort port) const;
    void setPortName(WhichPort port, const QString &value);

    BaudRateType baudRate(WhichPort port) const;
    void setBaudRate(WhichPort port, BaudRateType value);

    DataBitsType dataBits(WhichPort port) const;
    void setDataBits(WhichPort port, DataBitsType value);

    StopBitsType stopBits(WhichPort port) const;
    void setStopBits(WhichPort port, StopBitsType value);

    ParityType parity(WhichPort port) const;
    void setParity(WhichPort port, ParityType value);

    FlowType flowControl(WhichPort port) const;
    void setFlowControl(WhichPort port, FlowType value);
    // TODO: add timeout setting?

    bool debugMode(WhichPort port) const;
    void setDebugMode(WhichPort port, bool value);

    // a convenience: get/set all params above in one call
    PortSettingsEx portSettigns(WhichPort port) const;
    void setPortSettings(WhichPort port, PortSettingsEx value);

    // Log settings
    bool isLevelEnabled(Logger::Level level) const;
    void setLevelEnabled(Logger::Level level, bool value);

    // GUI settings

    bool isTableShown() const;
    void setTableShown(bool value);

    bool isSettingsShown() const;
    void setSettingsShown(bool value);

    bool isStatsShown() const;
    void setStatsShown(bool value);

    bool isPlotFixedScale() const;
    void setPlotFixedScale(bool value);

    int  plotFixedScaleMax() const;
    void setPlotFixedScaleMax(int value);
    int  plotFixedScaleMin() const;
    void setPlotFixedScaleMin(int value);

    int  plotHistorySecs() const;
    void setPlotHistorySecs(int value);
    
signals:
    
public slots:

private:
    QSettings settings;

    Q_DISABLE_COPY(Settings)
};

#endif // SETTINGS_H
