#ifndef PERFORMANCEREPORTER_H
#define PERFORMANCEREPORTER_H

#include <QObject>
#include "logger.h"

// TODO: possibility to disable the reporter
class PerformanceReporter : public QObject
{
    Q_OBJECT
public:
    explicit PerformanceReporter(QString description, Logger::Level level = Logger::Info, QObject *parent = 0);

    /**
     * @brief Adds another time measurement and recomputes min/max/avg
     * @param ms - measurement in milliseconds
     */
    void addMeasurement(double time);

    /**
     * @brief Reports current statistics to log
     */
    void reportResults();

    // TODO: something more elegant
    static void flushDebug();
    
private:
    const QString description;
    const Logger::Level logLevel;
    const QString mode;

    int measurementsCount;
    double minTime;
    double maxTime;
    double avgTime;

    void reportTime(QString prefix, double time);
    
    Q_DISABLE_COPY(PerformanceReporter)
};

#endif // PERFORMANCEREPORTER_H
