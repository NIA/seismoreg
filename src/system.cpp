#include "system.h"
#include "logger.h"

#ifdef Q_OS_WIN
#include <windows.h>
void System::setSystemTime(QDateTime t) {
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
        Logger::error(tr("Failed to set local time, error code %1").arg(err));
    }
}

#else

void System::setSystemTime(QDateTime t) {
    Logger::error("System::setSystemTime not yet implemented for non-Windows!");
}

#endif // Q_OS_WIN

