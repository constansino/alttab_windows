#ifndef ALTTAB_WINDOWS_QTWIN_H
#define ALTTAB_WINDOWS_QTWIN_H

#include <QWidget>

/// WinExtra module has been removed from Qt6, QAQ
namespace QtWin {
    void taskbarDeleteTab(QWidget* window);
    QPixmap fromHICON(HICON icon);
} // QtWin

#endif // ALTTAB_WINDOWS_QTWIN_H


