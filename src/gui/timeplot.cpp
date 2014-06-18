#include "timeplot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_draw.h"
#include "qwt_date.h"
#include "qwt_date_scale_draw.h"
#include "qwt_date_scale_engine.h"
#include "../logger.h"

#define RENDER_ANTIALISED 0

namespace {
    const QColor GRID_COLOR(192, 192, 192);
    const QColor CURVE_COLOR(20, 20, 100);
    const QColor BG_COLOR = Qt::white;
    const QColor CURVE_FILL = Qt::transparent; // may be some color, but currently disabled
    const qreal  CURVE_WIDTH = 2;
    const double ZOOM_FACTOR = 1.2;
    const double MOVE_FACTOR = 0.05;
    const double MIN_SCALE = 1/(ZOOM_FACTOR - 1); // minimum scale so that we can zoom out from it

    class RoundedScaleDraw : public QwtScaleDraw {
    public:
        QwtText label(double value) const override {
            return QLocale().toString(value, 'f', 0);
        }
    };

    class TimeScaleDraw : public QwtDateScaleDraw {
    protected:
        QString dateFormatOfDate(const QDateTime &, QwtDate::IntervalType) const override {
            // Same format for all
            return "hh:mm:ss";
        }
    };
}

TimePlot::TimePlot(QWidget *parent) :
    QwtPlot(parent), channel(0), pointsPerSec(POINTS_PER_SEC_DEFAULT),
    historySeconds(HISTORY_SECONDS_DEFAULT), fixedScale(FIXED_SCALE_DEFAULT),
    fixedScaleMax(FIXED_SCALE_MAX_DEFAULT), fixedScaleMin(FIXED_SCALE_MIN_DEFAULT)
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

void TimePlot::setPointsPerSec(int value) {
    if (value <= MAX_POINTS_PER_SEC) {
        pointsPerSec = value;
    } else {
        pointsPerSec = MAX_POINTS_PER_SEC;
    }
}

void TimePlot::setFixedScaleY(bool fixed) {
    fixedScale = fixed;
    setAxisAutoScale(yLeft, ! fixed);
    if (fixedScale) {
        // Restore again previous value that may be lost after automatic scale
        setScaleY();
    }
}

void TimePlot::setFixedScaleYMax(double max) {
    fixedScaleMax = max;
    setScaleY();
}

void TimePlot::setFixedScaleYMin(double min) {
    fixedScaleMin = min;
    setScaleY();
}

void TimePlot::setScaleY() {
    // Fix errors if any:
    if (fixedScaleMax < fixedScaleMin) {
        qSwap(fixedScaleMax, fixedScaleMin);
    }
    if (qAbs(fixedScaleMax - fixedScaleMin) < MIN_SCALE) {
        fixedScaleMax = fixedScaleMin + MIN_SCALE;
    }
    // Change QwtPlot settings, if currently in fixed scale mode
    if (fixedScale) {
        setAxisScale(yLeft, fixedScaleMin, fixedScaleMax);
    }
}

void TimePlot::zoomIn() {
    zoom(ZOOM_FACTOR);
}

void TimePlot::zoomOut() {
    zoom(1/ZOOM_FACTOR);
}

void TimePlot::zoom(double factor)
{
    double mid = (fixedScaleMin + fixedScaleMax) / 2;
    double amp = qAbs(fixedScaleMax - fixedScaleMin) / 2;

    amp /= factor;
    fixedScaleMax = mid + amp;
    fixedScaleMin = mid - amp;
    setScaleY();
    emit zoomChanged(fixedScaleMin, fixedScaleMax);
}

void TimePlot::moveUp() {
    move(MOVE_FACTOR);
}

void TimePlot::moveDown() {
    move(-MOVE_FACTOR);
}

void TimePlot::move(double factor) {
    double delta = (fixedScaleMax - fixedScaleMin)*factor;
    fixedScaleMin += delta;
    fixedScaleMax += delta;
    setScaleY();
    emit zoomChanged(fixedScaleMin, fixedScaleMax);
}

void TimePlot::resetZoom() {
    fixedScaleMin = FIXED_SCALE_MIN_DEFAULT;
    fixedScaleMax = FIXED_SCALE_MAX_DEFAULT;
    setScaleY();
    emit zoomChanged(fixedScaleMin, fixedScaleMax);
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
    setCanvasBackground(BG_COLOR);
}

void TimePlot::initCurve() {
    curve = new QwtPlotCurve;
    curve->setBrush(CURVE_FILL);
    curve->setPen(CURVE_COLOR, CURVE_WIDTH);
    curve->setOrientation(Qt::Vertical);
#if RENDER_ANTIALISED
    curve->setRenderHint(QwtPlotCurve::RenderAntialiased);
#endif
    curve->attach(this);

}

int TimePlot::maxBufferSize() {
    /* How many points should we store to be able to plot last historySeconds seconds?
     * It is historySeconds*pointsPerSec, but reserve one more second just to have some margin.
     */
    return (historySeconds+1)*pointsPerSec;
}


QVector<QPointF> TimePlot::itemsToPoints(TimeStampsVector timestamps, DataVector items, unsigned ch) {
    int itemsCount = items.count();
    int timestampsCount = timestamps.count();
    if (timestampsCount != itemsCount) {
        Logger::warning(tr("Unequal size of timestamps and items: %1 vs %2").arg(timestampsCount).arg(itemsCount));
        itemsCount = qMin(itemsCount, timestampsCount);
    }

    int pointsCount = itemsCount;
    int skip = 1;
    if (itemsCount > pointsPerSec) {
        skip = qCeil( double(itemsCount) / pointsPerSec);
        pointsCount = qCeil( double(itemsCount) / skip );
        Logger::trace(QString("skip = %1, pc = %2").arg(skip).arg(pointsCount));
    }

    QVector<QPointF> data(pointsCount);
    for(int i = 0, p = 0; (i < itemsCount) && (p < pointsCount); ++p, i += skip) {
        data[p] = QPointF(timestamps[i], items[i].byChannel[ch]);
    }

    return data;
}

