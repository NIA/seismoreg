#include "settings.h"

namespace {
    QString SETTINGS_FILE = "seismoreg.ini";

    /**
     * NB: keys are case-insensitive in INI format,
     * that's why it's better to have them all in
     * the same case to avoid collision
     */

    // Prefixes
    QString CORE_PREFIX = "core/";
    QString GUI_PREFIX  = "gui/";

    // Keys
    QString PORT_ADC = CORE_PREFIX + "port_adc";
    QString PORT_GPS = CORE_PREFIX + "port_gps";
    QString TABLE_SHOWN    = GUI_PREFIX + "table_shown";
    QString SETTINGS_SHOWN = GUI_PREFIX + "settings_shown";
    QString STATS_SHOWN    = GUI_PREFIX + "stats_shown";

    // Default values
    QString PORT_ADC_DEFAULT = "TEST";
    QString PORT_GPS_DEFAULT = "TEST";
    bool TABLE_SHOWN_DEFAULT    = false;
    bool SETTINGS_SHOWN_DEFAULT = true;
    bool STATS_SHOWN_DEFAULT    = true;
}

Settings::Settings(QObject *parent) :
    QObject(parent), settings(SETTINGS_FILE, QSettings::IniFormat, this)
{
}

QString Settings::portNameADC() {
    return settings.value(PORT_ADC, PORT_ADC_DEFAULT).toString();
}
void Settings::setPortNameADC(const QString &port) {
    settings.setValue(PORT_ADC, port);
}

QString Settings::portNameGPS() {
    return settings.value(PORT_GPS, PORT_GPS_DEFAULT).toString();
}
void Settings::setPortNameGPS(const QString &port) {
    settings.setValue(PORT_GPS, port);
}

bool Settings::isTableShown() {
    return settings.value(TABLE_SHOWN, TABLE_SHOWN_DEFAULT).toBool();
}
void Settings::setTableShown(bool value) {
    settings.setValue(TABLE_SHOWN, value);
}

bool Settings::isSettingsShown() {
    return settings.value(SETTINGS_SHOWN, SETTINGS_SHOWN_DEFAULT).toBool();
}
void Settings::setSettingsShown(bool value) {
    settings.setValue(SETTINGS_SHOWN, value);
}

bool Settings::isStatsShown() {
    return settings.value(STATS_SHOWN, STATS_SHOWN_DEFAULT).toBool();
}
void Settings::setStatsShown(bool value) {
    settings.setValue(STATS_SHOWN, value);
}
