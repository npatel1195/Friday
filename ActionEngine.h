#pragma once
#include <QString>
#include <QDesktopServices>
#include <QUrl>
#include <QMap>
#include <QProcess>
#include <QDebug>

class ActionEngine {
public:
    // Helper to get .exe name from common words
    static QString getExeName(const QString &inputName) {
        QString appName = inputName.toLower().trimmed();
        QMap<QString, QString> map;

        map["calc"] = "calc.exe";
        map["calculator"] = "calc.exe";
        map["notepad"] = "notepad.exe";
        map["chrome"] = "chrome.exe";
        map["spotify"] = "spotify.exe";
        map["word"] = "winword.exe";
        map["excel"] = "excel.exe";
        map["powerpoint"] = "powerpnt.exe";
        map["steam"] = "steam.exe";
        map["discord"] = "Discord.exe"; // Just the exe name for closing
        map["task manager"] = "taskmgr.exe";
        map["vlc"] = "vlc.exe";

        if (map.contains(appName)) return map[appName];
        if (!appName.endsWith(".exe")) return appName + ".exe";
        return appName;
    }

    // OPEN APP
    static void openApplication(const QString &inputName) {
        QString command = getExeName(inputName);

        // Special case for Discord Opening
        if(inputName.contains("discord")) command = "Update.exe --processStart Discord.exe";

        qDebug() << "🚀 Launching:" << command;

        QStringList args;
        args << "/c" << "start" << "" << command;
        QProcess::startDetached("cmd.exe", args);
    }

    // CLOSE APP (NEW FUNCTION)
    static void closeApplication(const QString &inputName) {
        QString exeName = getExeName(inputName);
        qDebug() << "💀 Killing process:" << exeName;

        // Windows command to force kill a process by name
        // taskkill /F /IM appname.exe
        QStringList args;
        args << "/F" << "/IM" << exeName;
        QProcess::startDetached("taskkill", args);
    }

    static void openWeb(const QString &url) {
        QString cleanUrl = url;
        if (!cleanUrl.startsWith("http")) cleanUrl = "https://" + cleanUrl;
        QDesktopServices::openUrl(QUrl(cleanUrl));
    }
};
