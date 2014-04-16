#include "system.h"
#include "logger.h"

namespace {
    const QString DATE_FMT = "yyyy-MM-ddThh:mm:ss.zzz";
}

#ifdef Q_OS_WIN
#include <windows.h>
bool System::setSystemTime(QDateTime t) {
    QDate date = t.date();
    QTime time = t.time();

    SYSTEMTIME st;
    st.wYear   = date.year();
    st.wMonth  = date.month();
    st.wDay    = date.day();
    st.wHour   = time.hour();
    st.wMinute = time.minute();
    st.wSecond = time.second();
    st.wMilliseconds = time.msec();
    if (SetLocalTime(&st) == 0) {
        DWORD err = GetLastError();
        Logger::error(tr("Failed to set system time, error code %1").arg(err));
        return false;
    } else {
        Logger::info(tr("System time set to %1").arg(t.toString(DATE_FMT)));
        return true;
    }
}

#else
#include <QProcess>

namespace {
    const QString DATE_CMD = "date --set=%1";
}
bool System::setSystemTime(QDateTime t) {
    QString dateStr = t.toString(DATE_FMT);
    int res = QProcess::execute(DATE_CMD.arg(dateStr));
    if (res != 0) {
        Logger::error(tr("Failed to set system time, exit code %1 (maybe you are not root?)").arg(res));
        return false;
    } else {
        Logger::info(tr("System time set to %1").arg(dateStr));
        return true;
    }
}

#endif // Q_OS_WIN

