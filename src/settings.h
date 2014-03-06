#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = 0);

    // Core settings
    QString portNameADC() const;
    void setPortNameADC(const QString &port);

    QString portNameGPS() const;
    void setPortNameGPS(const QString &port);

    int samplingFrequency() const;
    void setSamplingFrequency(int value);

    /**
     * This setting has no predefined default value and normally is not saved between sessions.
     * It is useful only for command-line usage.
     */
    QString saveFileNameOrDefault(const QString &defaultFileName) const;
    void setSaveFileName(const QString &value);

    // GUI settings

    bool isTableShown() const;
    void setTableShown(bool value);

    bool isSettingsShown() const;
    void setSettingsShown(bool value);

    bool isStatsShown() const;
    void setStatsShown(bool value);
    
signals:
    
public slots:

private:
    QSettings settings;
};

#endif // SETTINGS_H
