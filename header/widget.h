#ifndef ALTTAB_WINDOWS_WIDGET_H
#define ALTTAB_WINDOWS_WIDGET_H

#include <QWidget>
#include <Windows.h>
#include <QListWidget>
#include <QDebug>

struct WindowGroup;

struct WindowInfo {
    QString title;
    QString className;
    HWND hwnd = nullptr;
};

inline QDebug operator<<(QDebug dbg, const WindowInfo& info) {
    dbg.nospace() << "WindowInfo(" << info.title << ", " << info.className << ", " << info.hwnd << ")";
    return dbg.space();
}

Q_DECLARE_METATYPE(WindowInfo)
struct WindowGroup {
    WindowGroup() = default;
    void addWindow(const WindowInfo& window) { windows.append(window); }
    QString exePath;
    QIcon icon;
    QList<WindowInfo> windows;
};

Q_DECLARE_METATYPE(WindowGroup)

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

public:
    enum ForegroundChangeSource { WinEvent, Inner };
    Q_ENUM(ForegroundChangeSource)

public:
    explicit Widget(QWidget* parent = nullptr);
    QList<WindowGroup> prepareWindowGroupList();
    bool prepareListWidget();
    Q_INVOKABLE bool requestShow();
    void notifyForegroundChanged(HWND hwnd, ForegroundChangeSource source);
    Q_INVOKABLE void onGlobalKeyDown(int vkCode);

    HWND hWnd() { return (HWND) winId(); }
    bool isForeground() { return GetForegroundWindow() == hWnd(); }

    ~Widget() override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void rotateTaskbarWindowInGroup(const QString& exePath, bool forward, int windows);
    void clearGroupWindowOrder();

private:
    void confirmSelection();
    bool forceShow();
    void showLabelForItem(QListWidgetItem* item, QString text = QString());
    void setupLabelFont();
    auto getLastActiveGroupWindow(const QString& exePath) -> QPair<HWND, QDateTime>;
    auto getLastValidActiveGroupWindow(const WindowGroup& group) -> QPair<HWND, QDateTime>;
    void sortGroupWindows(QList<HWND>& windows, const QString& exePath);
    QList<HWND> buildGroupWindowOrder(const QString& exePath);
    static HWND rotateWindowInGroup(const QList<HWND>& windows, HWND current, bool forward = true);
    static HWND rotateNormalWindowInGroup(const QList<HWND>& windows, HWND current, bool forward = true);

private:
    Ui::Widget* ui;
    QListWidget* lw = nullptr;
    const QMargins ListWidgetMargin{24, 24, 24, 24};
    QHash<QString, QHash<HWND, QDateTime>> winActiveOrder;
    QList<HWND> groupWindowOrder;
    QPoint savedMousePos;
    HWND lastForegroundWindow = nullptr;
    QString bestTargetExe;
};

#endif // ALTTAB_WINDOWS_WIDGET_H