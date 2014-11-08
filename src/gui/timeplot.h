#ifndef TIMEPLOT_H
#define TIMEPLOT_H

#include <QDateTime>
#include "qwt_plot.h"
#include "../protocol.h"

class QwtPlotCurve;

/**
 * @brief The plot of one data series versus time
 *
 * A common element of GUI: plot with one curve,
 * with x-axis being time axis
 */
class TimePlot : public QwtPlot
{
    Q_OBJECT
public:
    explicit TimePlot(QWidget *parent = nullptr);

    /**
     * @brief Sets data for plot and replots it
     * @deprecated Better to receive portions of data via receiveData
     *
     * From each item of \a items array, takes all
     * values for the channel number \a ch and sets
     * as the curve samples.
     * @param timestamps - timestamps for \a items
     * @param items - data items (for all channels)
     * @param ch - number of channels to take
     */
    void setData(TimeStampsVector timestamps, DataVector items, unsigned ch);

    /**
     * @brief Sets raw data (already converted to QPointF) and replots
     * @param points - raw data
     */
    void setData(QVector<QPointF> points);

    void setChannel(unsigned ch) { channel = ch; }
    /**
     * @brief Sets points per second of plot.
     *
     * If more points received at once, some will be skipped so that
     * total number of plotted points is not more than pointsPerSec.
     *
     * @param value - number of points per second,
     * it can not be more than MAX_POINTS_PER_SEC
     * (for performance reasons)
     */
    void setPointsPerSec(int value);

public slots:
    /**
     * @brief Receives new data, adds them to the end, then removes old ones from the beginning
     * so that only data that fits on plot are saved. Then replots.
     *
     * @note Uses the channel number that was set via setChannel, and
     * points per second count that was set via setPointsPerSec.
     * Be sure to call this setters before you first invoke this slot.
     *
     * @param timestamps - a new portion of timestamps
     * @param items - a new portion of data items
     */
    void receiveData(TimeStampsVector timestamps, DataVector items);

    /**
     * @brief Clears what is currently stored in history buffer.
     *
     * This is useful when a new data series is initiated (e.g.
     * when protocol is closed and reopened)
     */
    void clearHistory() {
        buffer.clear();
    }

    // Settings

    void setHistorySecs(double secs);
    int  historySecs() { return historySeconds; }
    void setFixedScaleY(bool fixed);
    bool isFixedScaleY() { return fixedScale; }
    void setFixedScaleYMax(double max);
    double  fixedScaleYMax() { return fixedScaleMax; }
    void setFixedScaleYMin(double min);
    double  fixedScaleYMin() { return fixedScaleMin; }
    void fixCurrent();

    // easier interface to Y scale:
    void zoomIn();
    void zoomOut();
    void moveUp();
    void moveDown();
    void resetZoom();

signals:
    void zoomChanged(double newScaleYMin, double newScaleYMax);

public:
    constexpr static const double HISTORY_SECONDS_DEFAULT = 5;
    constexpr static const int    POINTS_PER_SEC_DEFAULT  = 200;
    constexpr static const int    MAX_POINTS_PER_SEC      = 500;
    constexpr static const double FIXED_SCALE_MAX_DEFAULT = 10000000;
    constexpr static const double FIXED_SCALE_MIN_DEFAULT = -FIXED_SCALE_MAX_DEFAULT;
    constexpr static const bool   FIXED_SCALE_DEFAULT     = false;

private:
    void initGrid();
    void initCurve();
    void setTimeRange(QVector<QPointF> points);
    /**
     * If \a fixedScale == true, apply values from fields \a fixedScaleMin and \a fixedScaleMax to plot
     */
    void setScaleY();

    /**
     * @brief The size depends on HISTORY_SECONDS constant and on pointsPerSec
     * @return the maximum size of buffer that is enough for plotting
     */
    int maxBufferSize();

    void zoom(double factor);
    void move(double factor);

    QVector<QPointF> itemsToPoints(TimeStampsVector timestamps, DataVector items, unsigned ch);

    QwtPlotCurve * curve;

    unsigned channel;
    int pointsPerSec;
    double historySeconds;
    bool fixedScale;
    double fixedScaleMax;
    double fixedScaleMin;
    QVector<QPointF> buffer;
};

#endif // TIMEPLOT_H
