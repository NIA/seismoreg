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
     *
     * From each item of \a items array, takes all
     * values for the channel number \a ch and sets
     * as the curve samples.
     * @param items
     * @param ch
     */
    void setData(DataVector items, unsigned ch);
    
private:
    void initGrid();
    void initCurve();

    QwtPlotCurve * curve;
};

#endif // TIMEPLOT_H
