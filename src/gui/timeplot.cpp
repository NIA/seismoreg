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
    QwtPlot(parent)
{
    setMinimumHeight(75);
    setAxisScaleDraw(yLeft,   new RoundedScaleDraw);
    setAxisScaleDraw(xBottom, new TimeScaleDraw);
    setAxisScaleEngine(xBottom, new QwtDateScaleEngine);

    initCurve();
    initGrid();
}

void TimePlot::setData(TimeStampsVector timestamps, DataVector items, unsigned ch) {
    QVector<QPointF> data;

    int itemsCount = items.count();
    int timestampsCount = timestamps.count();
    if (timestampsCount != itemsCount) {
        Logger::warning(tr("Unequal size of timestamps and items: %1 vs %2").arg(timestampsCount).arg(itemsCount));
        itemsCount = qMin(itemsCount, timestampsCount);
    }

    for(int i = 0; i < itemsCount; ++i) {
        // TODO: take not all?
        data << QPointF(QwtDate::toDouble(timestamps[i]), items[i].byChannel[ch]);
        ++i;
    }

    curve->setSamples(data);
    replot();
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
