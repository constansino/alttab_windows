#include <QApplication>
#include <windows.h>
#include <QTimer>
#include <QMessageBox>
#include <qoperatingsystemversion.h>
#include <QStyleHints>
#include "UpdateDialog.h"
#include "widget.h"
#include "utils/winEventHook.h"
#include "utils/Util.h"
#include "utils/TaskbarWheelHooker.h"
#include "utils/KeyboardHooker.h"
#include "utils/ComInitializer.h"
#include "utils/SingleApp.h"
#include "utils/SystemTray.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

void myMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    static QFile logFile(QApplication::applicationDirPath() + "/debug.log");
    if (!logFile.isOpen()) {
        logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
    QTextStream out(&logFile);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ") << msg << Qt::endl;
}

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    qInstallMessageHandler(myMessageOutput);
    SingleApp singleApp("alttab_windows-MrBeanCpp");
    if (singleApp.isRunning()) {
        qWarning() << "Another instance is running! Exit";
        QMessageBox::warning(nullptr, "Warning", "alttab_windows is already running!");
        return 0;
    }

    ComInitializer com; 
    qDebug() << qt_error_string(S_OK); 

    qDebug() << "isUserAdmin" << IsUserAnAdmin();
    qDebug() << "System Version" << QOperatingSystemVersion::current().version();
    sysTray.show(); 
    UpdateDialog::verifyUpdate(a); 

    QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    qApp->setQuitOnLastWindowClosed(false);
    auto* winSwitcher = new Widget;
    winSwitcher->prepareListWidget(); 

    QObject::connect(&a, &QApplication::aboutToQuit, []() {
        unhookWinEvent();
    });

    KeyboardHooker kbHooker(winSwitcher);
    TaskbarWheelHooker tbHooker;
    QObject::connect(&tbHooker, &TaskbarWheelHooker::tabWheelEvent,
                     winSwitcher, &Widget::rotateTaskbarWindowInGroup, Qt::QueuedConnection);
    QObject::connect(&tbHooker, &TaskbarWheelHooker::leaveTaskbar,
                     winSwitcher, &Widget::clearGroupWindowOrder, Qt::QueuedConnection);

    setWinEventHook([winSwitcher](DWORD event, HWND hwnd) {
        if (event == EVENT_SYSTEM_FOREGROUND) { 
            winSwitcher->notifyForegroundChanged(hwnd, Widget::WinEvent);
            auto className = Util::getClassName(hwnd);
            if (hwnd == GetForegroundWindow() && Util::isKeyPressed(VK_MENU) &&
                (className == "ForegroundStaging")) { 
                winSwitcher->requestShow();
            }
        }
    });

    return a.exec();
}