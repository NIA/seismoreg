#include "timeplot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_draw.h"
#include "qwt_date.h"
#include "qwt_date_scale_draw.h"
#include "qwt_date_scale_engine.h"
#include "../logger.h"

namespace {
    const QColor GRID_COLOR(128, 128, 128);
    const QColor CURVE_COLOR(40, 90, 180);
    const QColor CURVE_FILL = Qt::transparent; // may be some color, but currently disabled
    const qreal  CURVE_WIDTH = 2;

    class RoundedScaleDraw : public QwtScaleDraw {
    public:
        virtual QwtText label(double value) const {
            return QLocale().toString(value, 'f', 0);
        }
    };

    class TimeScaleDraw : public QwtDateScaleDraw {
    protected:
        virtual QString dateFormatOfDate(const QDateTime &, QwtDate::IntervalType) const {
            // Same format for all
            // TODO: too wide plots
            return "hh:mm:ss";
        }
    };
}

TimePlot::TimePlot(QWidget *parent) :
    QwtPlot(parent), channel(0), pointsPerSec(POINTS_PER_SEC_DEFAULT),
    historySeconds(HISTORY_SECONDS_DEFAULT), fixedScaleMax(FIXED_SCALE_MAX_DEFAULT), fixedScale(FIXED_SCALE_DEFAULT)
{
    setMinimumHeight(75);
    setMinimumWidth(350);
    setAxisScaleDraw(yLeft,   new RoundedScaleDraw);
    setAxisScaleDraw(xBottom, new TimeScaleDraw);
    setAxisScaleEngine(xBottom, new QwtDateScaleEngine);

    initCurve();
    initGrid();
}

void TimePlot::setData(TimeStampsVector timestamps, DataVector items, unsigned ch) {
    QVector<QPointF> points = itemsToPoints(timestamps, items, ch);
    setData(points);
}

void TimePlot::setData(QVector<QPointF> points) {
    curve->setSamples(points);

    setTimeRange(buffer);

    replot();
}

void TimePlot::setTimeRange(QVector<QPointF> points) {
    double xmax = points.isEmpty() ? QwtDate::toDouble(QDateTime::currentDateTime()) : points.last().x();
    double xmin = xmax - historySeconds*1000;
    setAxisScale(xBottom, xmin, xmax);
}

void TimePlot::setHistorySecs(double secs) {
    historySeconds = secs;
    setTimeRange(buffer); // TODO: need to pass buffer or not?
}

void TimePlot::setFixedScaleY(bool fixed) {
    fixedScale = fixed;
    setAxisAutoScale(yLeft, ! fixed);
    if (fixedScale) {
        // Restore again previous value that may be lost after automatic scale
        setFixedScaleYMax(fixedScaleMax);
    }
}

void TimePlot::setFixedScaleYMax(double max) {
    fixedScaleMax = max;
    setAxisScale(yLeft, -max, +max);
}

void TimePlot::receiveData(TimeStampsVector timestamps, DataVector items) {
    // Add new points
    QVector<QPointF> newPoints = itemsToPoints(timestamps, items, channel);
    buffer += newPoints;

    // Strip old ones from the beginning
    int maxSize = maxBufferSize();
    if (buffer.size() > maxSize) {
         int excess = buffer.size() - maxSize;
         buffer.remove(0, excess);
    }
    setData(buffer);
}

void TimePlot::initGrid() {
    QwtPlotGrid * grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->setMajorPen(QPen(GRID_COLOR));
    grid->setMinorPen(QPen(GRID_COLOR, 1, Qt::DashLine));
    grid->attach(this);
}

void TimePlot::initCurve() {
    curve = new QwtPlotCurve;
    curve->setBrush(CURVE_FILL);
    curve->setPen(CURVE_COLOR, CURVE_WIDTH);
    curve->setOrientation(Qt::Vertical);
    curve->setRenderHint(QwtPlotCurve::RenderAntialiased);
    curve->attach(this);

}

int TimePlot::maxBufferSize() {
    /* How many points should we store to be able to plot last historySeconds seconds?
     * It is historySeconds*pointsPerSec, but reserve two times of this just to have some margin.
     */
    return 2*historySeconds*pointsPerSec;
}


QVector<QPointF> TimePlot::itemsToPoints(TimeStampsVector timestamps, DataVector items, unsigned ch) {
    int itemsCount = items.count();
    int timestampsCount = timestamps.count();
    if (timestampsCount != itemsCount) {
        Logger::warning(tr("Unequal size of timestamps and items: %1 vs %2").arg(timestampsCount).arg(itemsCount));
        itemsCount = qMin(itemsCount, timestampsCount);
    }

    QVector<QPointF> data(itemsCount);
    for(int i = 0; i < itemsCount; ++i) {
        data[i] = QPointF(QwtDate::toDouble(timestamps[i]), items[i].byChannel[ch]);
    }

    return data;
}

