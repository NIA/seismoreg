#include "performancereporter.h"
#include <qglobal.h>

#include <QDebug>  // for flushDebug
#include <QThread> // for flushDebug

/*
 * This file is based on performance_reporter.cpp from Crash-And-Squeeze,
 * a bachelor degree project by Ivan Novikov:
 * https://github.com/NIA/crash-and-squeeze/blob/master/Renderer/performance_reporter.cpp
 */

namespace
{
    inline QString releaseOrDebug() {
#ifdef QT_DEBUG
        return PerformanceReporter::tr("Debug");
#else
        return PerformanceReporter::tr("Release");
#endif
    }
}

PerformanceReporter::PerformanceReporter(QString description, Logger::Level level, QObject *parent) :
    QObject(parent), description(description), logLevel(level), mode(releaseOrDebug()),
    paused(false), beforePause(0),
    measurementsCount(0), minTime(0), maxTime(0), avgTime(0)
{
}

void PerformanceReporter::addMeasurement(double time) {
    if (measurementsCount <= 0) {
        minTime = maxTime = avgTime = time;
        measurementsCount = 1;
    } else {
        avgTime = (avgTime*measurementsCount + time) / (measurementsCount + 1);

        if (time < minTime) {
            minTime = time;
        }

        if (time > maxTime) {
            maxTime = time;
        }

        ++measurementsCount;
    }
}

void PerformanceReporter::reportResults() {
    if (measurementsCount > 0) {
        Logger::message(logLevel, tr("Performance for %1 in %2").arg(description).arg(mode));
        reportTime(tr("AVG"), avgTime);
        reportTime(tr("MAX"), maxTime);
        reportTime(tr("MIN"), minTime);
    }
}

void PerformanceReporter::reportTime(QString prefix, double time) {
    Logger::message(logLevel, tr("%1: %2 ms").arg(prefix, 4).arg(time, 7, 'f', 2));
}

void PerformanceReporter::flushDebug() {
    qDebug() << flush;
    QThread::msleep(500);
}
