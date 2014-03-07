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
    explicit TimePlot(QWidget *parent = 0);

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
    void setPointsPerSec(int value) { pointsPerSec = value; }

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
    
private:
    void initGrid();
    void initCurve();

    /**
     * @brief The size depends on HISTORY_SECONDS constant and on pointsPerSec
     * @return the maximum size of buffer that is enough for plotting
     */
    int maxBufferSize();

    QVector<QPointF> itemsToPoints(TimeStampsVector timestamps, DataVector items, unsigned ch);

    QwtPlotCurve * curve;

    unsigned channel;
    int pointsPerSec;
    QVector<QPointF> buffer;
};

#endif // TIMEPLOT_H
