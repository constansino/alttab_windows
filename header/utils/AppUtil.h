#ifndef ALTTAB_WINDOWS_APPUTIL_H
#define ALTTAB_WINDOWS_APPUTIL_H

#include <Windows.h>
#include <QIcon>

namespace AppUtil {
    HWND getAppFrameWindow(HWND hwnd);
    HWND getAppCoreWindow(HWND hwnd);
    bool isAppFrameWindow(HWND hwnd);
    QIcon getAppIcon(const QString& path);
    QString getExePathFromAppIdOrName(const QString& appid = QString(), const QString& appName = QString());

    inline const QString AppCoreWindowClass = "Windows.UI.Core.CoreWindow";
    inline const QString AppFrameWindowClass = "ApplicationFrameWindow";
    inline const QString AppManifest = "AppxManifest.xml";
}


#endif // ALTTAB_WINDOWS_APPUTIL_H


