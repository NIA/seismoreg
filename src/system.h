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
    /**
     * @brief Sets system time to given QDateTime
     * @param t - the date/time to be set
     * @return true if success, false if failed (error will be reported to Logger)
     */
    static bool setSystemTime(QDateTime t);
};

#endif // SYSTEM_H
