#ifndef SYSTEM_H
#define SYSTEM_H

#include <QDateTime>

/**
 * @brief System-specific features
 *
 * Just a set of static functions whose implementation is OS-dependent
 */
class System : public QObject
{
    Q_OBJECT
public:
    static void setSystemTime(QDateTime t);
};

#endif // SYSTEM_H
