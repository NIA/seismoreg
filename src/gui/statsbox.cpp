#include "statsbox.h"
#include "ui_statsbox.h"
#include "../logger.h"

StatsBox::StatsBox(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::StatsBox)
{
    ui->setupUi(this);
}

void StatsBox::setStats(DataType min, DataType max, DataType avg) {
    prevStats = curStats;
    curStats.min = min;
    curStats.max = max;
    curStats.avg = avg;

    updateWidgets();
}

void StatsBox::setStats(DataVector items, int ch) {
    if (items.isEmpty()) {
        Logger::warning(tr("Cannot calculate stats for empty data"));
        return;
    }

    DataType min, max;
    min = max = items[0].byChannel[ch];
    DataType avg = 0;
    foreach (DataItem item, items) {
        DataType val  = item.byChannel[ch];
        if (val < min) {
            min = val;
        }
        if (val > max) {
            max = val;
        }
        avg += val;
    }
    avg /= items.count();

    setStats(min, max, avg);
}

StatsBox::~StatsBox() {
    delete ui;
}

void StatsBox::updateWidgets() {
    updateWidget(ui->prevMin, prevStats.min);
    updateWidget(ui->prevMax, prevStats.max);
    updateWidget(ui->prevAvg, prevStats.avg);
    updateWidget(ui->curMin,  curStats.min);
    updateWidget(ui->curMax,  curStats.max);
    updateWidget(ui->curAvg,  curStats.avg);
}

void StatsBox::updateWidget(QLineEdit *w, DataType val) {
    w->setText(QString::number(val));
}
