#ifndef PERFORMANCEREPORTER_H
#define PERFORMANCEREPORTER_H

#include <QObject>
#include <QElapsedTimer>
#include "logger.h"

// TODO: possibility to disable the reporter
class PerformanceReporter : public QObject
{
    Q_OBJECT
public:
    explicit PerformanceReporter(QString description, Logger::Level level = Logger::Info, QObject *parent = nullptr);

    /**
     * @brief Adds another time measurement and recomputes min/max/avg
     *
     * You can use internal timer to easier measure time, \see start() and stop()
     * @param ms - measurement in milliseconds
     */
    void addMeasurement(double time);

    /**
     * @brief Starts internal timer, \see stop()
     */
    void start() {
        beforePause = 0;
        timer.start();
    }

    /**
     * @brief Pause timer, do not add measurement
     */
    void pause() {
        if (!paused) {
            beforePause += timer.elapsed();
            paused = true;
        }
    }

    /**
     * @brief Continue receiving after pause
     */
    void unpause() {
        if (paused) {
            paused = false;
            timer.start();
        }
    }

    /**
     * @brief adds the elapsed time of internal timer as measurement
     */
    void stop() {
        if (paused) {
            addMeasurement(beforePause);
        } else {
            addMeasurement(timer.elapsed() + beforePause);
        }
    }

    // TODO: not good to change description, but useful
    void setDescription(QString newDescription) {
        description = newDescription;
    }

    /**
     * @brief Reports current statistics to log
     */
    void reportResults();

    // TODO: something more elegant
    static void flushDebug();
    
private:
    QString description;
    const Logger::Level logLevel;
    const QString mode;

    QElapsedTimer timer;
    bool paused;
    qint64 beforePause;

    int measurementsCount;
    double minTime;
    double maxTime;
    double avgTime;

    void reportTime(QString prefix, double time);
    
    Q_DISABLE_COPY(PerformanceReporter)
};

#endif // PERFORMANCEREPORTER_H
