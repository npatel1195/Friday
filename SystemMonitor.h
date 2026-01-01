#pragma once
#include <QString>
#ifdef _WIN32
#include <windows.h>
#endif

class SystemMonitor {
public:
    static QString getActiveWindowTitle() {
#ifdef _WIN32
        char buf[256];
        if(GetWindowTextA(GetForegroundWindow(), buf, 256)) return QString::fromLocal8Bit(buf);
#endif
        return "";
    }
};
