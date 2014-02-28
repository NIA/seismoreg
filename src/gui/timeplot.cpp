#include "timeplot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_draw.h"

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

}

TimePlot::TimePlot(QWidget *parent) :
    QwtPlot(parent)
{
    setMinimumHeight(75);
    setAxisScaleDraw(QwtPlot::yLeft, new RoundedScaleDraw);

    initCurve();
    initGrid();
}

void TimePlot::setData(DataVector items, unsigned ch) {
    QVector<QPointF> data;
    int i = 0;
    foreach(DataItem item, items) {
        // TODO: 1) time axis 2) take not all?
        data << QPointF(i, item.byChannel[ch]);
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
