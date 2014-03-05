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
    QString portNameADC();
    void setPortNameADC(const QString &port);

    QString portNameGPS();
    void setPortNameGPS(const QString &port);

    // GUI settings

    bool isTableShown();
    void setTableShown(bool value);

    bool isSettingsShown();
    void setSettingsShown(bool value);

    bool isStatsShown();
    void setStatsShown(bool value);
    
signals:
    
public slots:

private:
    QSettings settings;
};

#endif // SETTINGS_H
