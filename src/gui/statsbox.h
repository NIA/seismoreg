#ifndef STATSBOX_H
#define STATSBOX_H

#include "../protocol.h"
#include <QFrame>
#include <QLineEdit>

namespace Ui {
class StatsBox;
}

class StatsBox : public QFrame
{
    Q_OBJECT

public:
    explicit StatsBox(QWidget *parent = 0);

    /**
     * @brief Sets current stats:
     * @param min - minimum
     * @param max - maximum
     * @param avg - average
     */
    void setStats(DataType min, DataType max, DataType avg);

    /**
     * @brief Calculates and sets current stats.
     *
     * Overloaded version for ease of use:
     * takes an array of items and calculates the stats
     * @param items - data vector
     * @param ch - number of channel to calculate stats for
     */
    void setStats(DataVector items, int ch);

    ~StatsBox();
    
private:
    Ui::StatsBox *ui;

    void updateWidgets();
    void updateWidget(QLineEdit * w, DataType val);

    struct Stats {
        DataType min;
        DataType max;
        DataType avg;
        // Default constructor to avoid uninitialized values
        Stats() : min(0), max(0), avg(0) {}
    } prevStats, curStats;
};

#endif // STATSBOX_H
