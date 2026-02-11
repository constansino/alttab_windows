#include "../header/widget.h"
#include "ui_Widget.h"
#include "utils/Util.h"
#include <QDebug>
#include <QWindow>
#include <QScreen>
#include "utils/setWindowBlur.h"
#include "utils/IconOnlyDelegate.h"
#include <QPainter>
#include <QPen>
#include <QDateTime>
#include "utils/QtWin.h"
#include <QWheelEvent>
#include <QTimer>
#include <QMetaEnum>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QMenu>
#include <QApplication>
#include "utils/SystemTray.h"
#include "utils/ConfigManager.h"

Widget::Widget(QWidget* parent) : QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
    lw = ui->listWidget;
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground); 
    QtWin::taskbarDeleteTab(this);
    setWindowTitle("alttab_windows");

    Util::setWindowRoundCorner(this->hWnd());
    setWindowBlur(hWnd());

    setupLabelFont();

    lw->setViewMode(QListView::IconMode);
    lw->setMovement(QListView::Static);
    lw->setFlow(QListView::LeftToRight);
    lw->setWrapping(false);
    lw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lw->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lw->setIconSize({64, 64});
    lw->setGridSize({80, 80});
    lw->setFixedHeight(lw->gridSize().height());
    lw->setUniformItemSizes(true);
    lw->setMouseTracking(true); 
    lw->setStyleSheet(R"(
        QListWidget {
            background-color: transparent;
            border: none;
            outline: none;
        }
    )");
    
    lw->setItemDelegate(new IconOnlyDelegate(lw));
    lw->installEventFilter(this);
    lw->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(lw, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* cur, QListWidgetItem*) {
        if (cur) showLabelForItem(cur);
    });
    
    connect(lw, &QListWidget::itemEntered, this, [this](QListWidgetItem* item) {
        if (item) showLabelForItem(item);
    });

    connect(lw, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto item = lw->itemAt(pos);
        if (!item) return;
        auto group = item->data(Qt::UserRole).value<WindowGroup>();
        HWND hwnd = group.windows.isEmpty() ? nullptr : group.windows.first().hwnd;
        QString fileName = QFileInfo(group.exePath).fileName();

        QMenu menu(this);
        menu.setStyleSheet("QMenu{background-color:rgb(45,45,45); color:rgb(220,220,220); border:1px solid black;}"
                           "QMenu:selected{background-color:rgb(60,60,60);}");

        menu.addAction("Close Window", [hwnd] { Util::closeWindow(hwnd); });
        menu.addAction("Quit App (Force)", [hwnd] { Util::killProcess(hwnd); });
        menu.addSeparator();
        
        auto* pinMenu = menu.addMenu("Pin to Position");
        auto pinnedApps = cfg.getPinnedApps();
        int currentPin = pinnedApps.value(fileName, -1);
        for (int i = 0; i < 10; ++i) {
            auto* act = pinMenu->addAction(QString("Position %1").arg(i + 1));
            act->setCheckable(true);
            act->setChecked(currentPin == i);
            connect(act, &QAction::triggered, [this, fileName, i, currentPin] {
                if (currentPin == i) cfg.setPinnedApp(fileName, -1);
                else cfg.setPinnedApp(fileName, i);
                prepareListWidget();
            });
        }

        menu.addSeparator();
        auto shortcuts = cfg.getAppShortcuts();
        QString currentKey = shortcuts.value(fileName);
        QString shortcutText = currentKey.isEmpty() ? "Set Shortcut..." : QString("Shortcut: %1").arg(currentKey);
        menu.addAction(shortcutText, [this, fileName, currentKey] {
            QDialog dlg(this);
            dlg.setWindowTitle("Set Shortcut: " + fileName);
            dlg.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
            auto* layout = new QVBoxLayout(&dlg);
            layout->addWidget(new QLabel("Press keys:"));
            auto* editor = new QKeySequenceEdit(&dlg);
            if (!currentKey.isEmpty()) editor->setKeySequence(QKeySequence(currentKey));
            layout->addWidget(editor);
            auto* cbGlobal = new QCheckBox("Global Shortcut", &dlg);
            cbGlobal->setChecked(cfg.isShortcutGlobal(fileName));
            layout->addWidget(cbGlobal);
            auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset, &dlg);
            layout->addWidget(btnBox);
            connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
            connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
            connect(btnBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, [&] { editor->clear(); dlg.accept(); });
            if (dlg.exec() == QDialog::Accepted) {
                cfg.setAppShortcut(fileName, editor->keySequence().toString(QKeySequence::PortableText));
                cfg.setShortcutGlobal(fileName, cbGlobal->isChecked());
            }
        });
        menu.exec(lw->mapToGlobal(pos));
    });

    connect(lw, &QListWidget::itemClicked, this, &Widget::confirmSelection);
    connect(lw, &QListWidget::itemActivated, this, &Widget::confirmSelection);

    connect(qApp, &QApplication::focusWindowChanged, this, [this](QWindow* focusWindow) {
        if (focusWindow == nullptr) {
            if (this->findChild<QMenu*>() || this->findChild<QDialog*>()) return;
            if (!this->underMouse()) hide();
        }
    });
}

Widget::~Widget() { delete ui; }

void Widget::confirmSelection() {
    if (this->isVisible()) {
        if (cfg.getMouseWarp() && !savedMousePos.isNull()) {
            QCursor::setPos(savedMousePos);
            savedMousePos = QPoint();
        }
        if (auto item = lw->currentItem()) {
            auto group = item->data(Qt::UserRole).value<WindowGroup>();
            if (!group.windows.empty()) {
                WindowInfo targetWin = group.windows.at(0);
                const auto lastActive = getLastActiveGroupWindow(group.exePath).first;
                for (auto& info: group.windows) { if (info.hwnd == lastActive) { targetWin = info; break; } }
                if (targetWin.hwnd) Util::switchToWindow(targetWin.hwnd);
            }
        }
        hide();
    }
}

void Widget::keyPressEvent(QKeyEvent* event) {
    auto key = event->key();
    auto modifiers = event->modifiers();
    if (key == Qt::Key_Tab) {
        auto i = lw->currentRow();
        bool isShiftPressed = (modifiers & Qt::ShiftModifier);
        auto index = (i - (2 * isShiftPressed - 1) + lw->count()) % lw->count();
        lw->setCurrentRow(index);
    } else if (key == Qt::Key_QuoteLeft && (modifiers & Qt::AltModifier)) {
        if (this->isVisible() && !this->isMinimized()) { hide(); return; }
        auto foreWin = GetForegroundWindow();
        if (groupWindowOrder.isEmpty()) groupWindowOrder = buildGroupWindowOrder(Util::getWindowProcessPath(foreWin));
        if (auto nextWin = rotateWindowInGroup(groupWindowOrder, foreWin, !(modifiers & Qt::ShiftModifier))) Util::switchToWindow(nextWin, true);
    } else if (key == Qt::Key_Up || key == Qt::Key_Down) {
        if (auto item = lw->currentItem()) {
            auto center = lw->visualItemRect(item).center();
            auto wheelEvent = new QWheelEvent(center, lw->mapToGlobal(center), {}, {key == Qt::Key_Up ? 120 : -120, 0}, Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            QApplication::postEvent(lw, wheelEvent);
        }
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        confirmSelection();
        return;
    } else if (key == Qt::Key_Escape) {
        if (cfg.getMouseWarp() && !savedMousePos.isNull()) { QCursor::setPos(savedMousePos); savedMousePos = QPoint(); }
        hide();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool Widget::forceShow() {
    setWindowOpacity(0.005);
    showMinimized();
    showNormal();
    setWindowOpacity(1);
    return isForeground();
}

void Widget::showLabelForItem(QListWidgetItem* item, QString text) {
    if (!item) return;
    auto group = item->data(Qt::UserRole).value<WindowGroup>();
    if (text.isNull()) text = Util::getFileDescription(group.exePath);
    ui->label->setText(text);
    ui->label->adjustSize();
    auto itemRect = lw->visualItemRect(item);
    auto center = itemRect.center() + QPoint(0, itemRect.height() / 2 + ListWidgetMargin.bottom() / 2);
    center = lw->mapTo(this, center);
    auto labelRect = ui->label->rect();
    labelRect.moveCenter(center);
    auto bound = this->rect().marginsRemoved({5, 0, 5, 0});
    labelRect.moveRight(qMin(labelRect.right(), bound.right()));
    labelRect.moveLeft(qMax(labelRect.left(), bound.left()));
    ui->label->move(labelRect.topLeft());
}

void Widget::setupLabelFont() {
    static auto reloadLabelFontCfg = [this] {
        auto labelFont = ui->label->font();
        labelFont.setPointSize(cfg.get("label/font_size", 10).toInt());
        ui->label->setFont(labelFont);
    };
    reloadLabelFontCfg();
    connect(&cfg, &ConfigManager::configEdited, this, [] { reloadLabelFontCfg(); });
}

void Widget::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Alt) {
        groupWindowOrder.clear();
        if (cfg.getHoldMode()) confirmSelection();
    }
    QWidget::keyReleaseEvent(event);
}

void Widget::hideEvent(QHideEvent* event) { QWidget::hideEvent(event); }

void Widget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(25, 25, 25, 120));
    painter.drawRect(rect());
}

void Widget::notifyForegroundChanged(HWND hwnd, ForegroundChangeSource source) {
    if (hwnd == this->hWnd()) return;
    if (!Util::isWindowAcceptable(hwnd, source == WinEvent)) return;
    auto path = Util::getWindowProcessPath(hwnd);
    winActiveOrder[path].insert(hwnd, QDateTime::currentDateTime());
}

QList<WindowGroup> Widget::prepareWindowGroupList() {
    QMap<QString, WindowGroup> winGroupMap;
    const auto list = Util::listValidWindows();
    for (auto hwnd: list) {
        if (hwnd == this->hWnd()) continue;
        auto path = Util::getWindowProcessPath(hwnd);
        if (path.isEmpty()) continue;
        auto& winGroup = winGroupMap[path];
        if (winGroup.exePath.isEmpty()) {
            winGroup.exePath = path;
            winGroup.icon = Util::getCachedIcon(path, hwnd);
        }
        winGroup.addWindow({Util::getWindowTitle(hwnd), Util::getClassName(hwnd), hwnd});
    }
    auto winGroupList = winGroupMap.values();
    std::sort(winGroupList.begin(), winGroupList.end(), [this](const WindowGroup& a, const WindowGroup& b) {
        auto timeA = getLastValidActiveGroupWindow(a).second;
        auto timeB = getLastValidActiveGroupWindow(b).second;
        if (timeA.isNull() && timeB.isNull()) return false;
        if (timeA.isValid() && timeB.isValid()) return timeA > timeB;
        return timeA.isValid();
    });

    bestTargetExe = "";
    DWORD forePid = 0;
    if (lastForegroundWindow) GetWindowThreadProcessId(lastForegroundWindow, &forePid);
    for (const auto& group : winGroupList) {
        DWORD itemPid = 0;
        if (!group.windows.isEmpty()) GetWindowThreadProcessId(group.windows.first().hwnd, &itemPid);
        if (forePid == 0 || itemPid != forePid) {
            bestTargetExe = group.exePath;
            break;
        }
    }

    auto pinnedApps = cfg.getPinnedApps();
    if (!pinnedApps.isEmpty()) {
        QMap<int, WindowGroup> pinned;
        QMutableListIterator<WindowGroup> it(winGroupList);
        while (it.hasNext()) {
            auto& group = it.next();
            QString name = QFileInfo(group.exePath).fileName();
            if (pinnedApps.contains(name)) { pinned.insert(pinnedApps.value(name), group); it.remove(); }
        }
        QList<WindowGroup> finalList;
        int normalIdx = 0;
        int total = winGroupList.size() + pinned.size();
        for (int i = 0; i < total; ++i) {
            if (pinned.contains(i)) finalList.append(pinned.value(i));
            else if (normalIdx < winGroupList.size()) finalList.append(winGroupList.at(normalIdx++));
        }
        while (normalIdx < winGroupList.size()) finalList.append(winGroupList.at(normalIdx++));
        winGroupList = finalList;
    }
    return winGroupList;
}

bool Widget::prepareListWidget() {
    auto winGroupList = prepareWindowGroupList();
    lw->clear();
    for (auto& winGroup: winGroupList) {
        auto item = new QListWidgetItem(winGroup.icon, {});
        item->setData(Qt::UserRole, QVariant::fromValue(winGroup));
        item->setSizeHint(lw->gridSize());
        lw->addItem(item);
    }
    if (auto firstItem = lw->item(0)) {
        auto firstRect = lw->visualItemRect(firstItem);
        auto width = lw->gridSize().width() * lw->count() + (firstRect.x() - lw->frameWidth());
        lw->setFixedWidth(width);
        auto screen = QGuiApplication::primaryScreen();
        if (cfg.getDisplayMonitor() == MouseMonitor) screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (!screen) return false;
        auto lwRect = lw->rect();
        auto thisRect = lwRect.marginsAdded(ListWidgetMargin);
        thisRect.moveCenter(screen->geometry().center());
        this->setGeometry(thisRect);
        lwRect.moveCenter(this->rect().center());
        lw->move(lwRect.topLeft());
    } else return false;

    if (lw->count() > 0) {
        int targetRow = 0;
        if (!bestTargetExe.isEmpty()) {
            for (int i = 0; i < lw->count(); ++i) {
                if (lw->item(i)->data(Qt::UserRole).value<WindowGroup>().exePath == bestTargetExe) { targetRow = i; break; }
            }
        }
        lw->setCurrentRow(targetRow);
    }
    return true;
}

bool Widget::requestShow() {
    HWND currentFore = GetForegroundWindow();
    QString className = Util::getClassName(currentFore);
    if (currentFore != this->hWnd() && className != "ForegroundStaging" && className != "XamlExplorerHostIslandWindow") {
        lastForegroundWindow = currentFore;
    }
    if (cfg.getMouseWarp()) savedMousePos = QCursor::pos();
    const bool ok = prepareListWidget() && forceShow();
    if (ok && cfg.getMouseWarp()) {
        QTimer::singleShot(0, this, [this]() {
            if (!this->isVisible()) return;
            if (auto item = lw->currentItem()) {
                auto center = lw->visualItemRect(item).center();
                QCursor::setPos(lw->mapToGlobal(center));
            }
        });
    }
    return ok;
}

void Widget::onGlobalKeyDown(int vkCode) {
    int qtKey = 0;
    if (vkCode >= 0x30 && vkCode <= 0x39) qtKey = vkCode;
    else if (vkCode >= 0x41 && vkCode <= 0x5A) qtKey = vkCode;
    else if (vkCode >= VK_F1 && vkCode <= VK_F12) qtKey = Qt::Key_F1 + (vkCode - VK_F1);
    if (qtKey == 0) return;
    if (GetKeyState(VK_CONTROL) & 0x8000) qtKey += Qt::CTRL;
    if (GetKeyState(VK_MENU) & 0x8000) qtKey += Qt::ALT;
    if (GetKeyState(VK_SHIFT) & 0x8000) qtKey += Qt::SHIFT;
    if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000)) qtKey += Qt::META;
    QString pressedStr = QKeySequence(qtKey).toString(QKeySequence::PortableText);
    QString rawKeyStr = QKeySequence(vkCode).toString(QKeySequence::PortableText);
    auto shortcuts = cfg.getAppShortcuts();
    for (auto it = shortcuts.begin(); it != shortcuts.end(); ++it) {
        QString recorded = it.value();
        QString targetExeName = it.key();
        if (recorded.compare(pressedStr, Qt::CaseInsensitive) == 0 || recorded.compare(rawKeyStr, Qt::CaseInsensitive) == 0 || (recorded.length() == 1 && pressedStr.endsWith(recorded, Qt::CaseInsensitive))) {
            if (this->isVisible()) {
                for (int i = 0; i < lw->count(); ++i) {
                    if (QFileInfo(lw->item(i)->data(Qt::UserRole).value<WindowGroup>().exePath).fileName().compare(targetExeName, Qt::CaseInsensitive) == 0) {
                        lw->setCurrentRow(i); confirmSelection(); return;
                    }
                }
            } else if (cfg.isShortcutGlobal(targetExeName)) {
                HWND foreWin = GetForegroundWindow();
                const auto allWindows = Util::listValidWindows();
                HWND firstMatchHwnd = nullptr; bool isAlreadyForeground = false;
                for (HWND hwnd : allWindows) {
                    QString path = Util::getWindowProcessPath(hwnd);
                    if (QFileInfo(path).fileName().compare(targetExeName, Qt::CaseInsensitive) == 0) {
                        if (!firstMatchHwnd) firstMatchHwnd = hwnd;
                        if (hwnd == foreWin) { isAlreadyForeground = true; break; }
                    }
                }
                if (isAlreadyForeground) ShowWindow(foreWin, SW_MINIMIZE);
                else if (firstMatchHwnd) Util::switchToWindow(firstMatchHwnd, true);
                return;
            }
        }
    }
}

auto Widget::getLastActiveGroupWindow(const QString& exePath) -> QPair<HWND, QDateTime> {
    auto hwndOrder = winActiveOrder.value(exePath);
    if (hwndOrder.isEmpty()) return {nullptr, QDateTime()};
    auto iter = std::max_element(hwndOrder.begin(), hwndOrder.end());
    return {iter.key(), iter.value()};
}

auto Widget::getLastValidActiveGroupWindow(const WindowGroup& group) -> QPair<HWND, QDateTime> {
    if (group.windows.isEmpty()) return {nullptr, QDateTime()};
    auto hwndOrder = winActiveOrder.value(group.exePath);
    if (hwndOrder.isEmpty()) return {nullptr, QDateTime()};
    QList<HWND> windows;
    for (auto& info: group.windows) windows << info.hwnd;
    sortGroupWindows(windows, group.exePath);
    if (windows.isEmpty()) return {nullptr, QDateTime()};
    if (auto time = hwndOrder.value(windows.first()); !time.isNull()) return {windows.first(), time};
    return {nullptr, QDateTime()};
}

void Widget::sortGroupWindows(QList<HWND>& windows, const QString& exePath) {
    if (windows.isEmpty()) return;
    auto activeOrdMap = winActiveOrder.value(exePath);
    if (activeOrdMap.isEmpty()) return;
    std::sort(windows.begin(), windows.end(), [&activeOrdMap](HWND a, HWND b) { return activeOrdMap.value(a) > activeOrdMap.value(b); });
}

QList<HWND> Widget::buildGroupWindowOrder(const QString& exePath) {
    auto windows = Util::listValidWindows(exePath);
    sortGroupWindows(windows, exePath);
    return windows;
}

bool Widget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == lw && event->type() == QEvent::Wheel) {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);
        auto cursorPos = wheelEvent->position().toPoint();
        if (auto item = lw->itemAt(cursorPos)) {
            if (lw->currentItem() != item) lw->setCurrentItem(item);
            auto windowGroup = item->data(Qt::UserRole).value<WindowGroup>();
            if (windowGroup.windows.isEmpty()) return false;
            static QListWidgetItem* lastItem = nullptr;
            static HWND hwnd = nullptr;
            if (lastItem != item) { lastItem = item; hwnd = nullptr; groupWindowOrder.clear(); }
            auto targetExe = windowGroup.exePath;
            static bool isLastRollUp = true;
            bool isRollUp = wheelEvent->angleDelta().x() > 0;
            if (groupWindowOrder.isEmpty()) groupWindowOrder = buildGroupWindowOrder(targetExe);
            if (!hwnd) hwnd = groupWindowOrder.first();
            else { if (isLastRollUp == isRollUp) hwnd = rotateWindowInGroup(groupWindowOrder, hwnd, isRollUp); }
            isLastRollUp = isRollUp;
            HWND nextFocus = hwnd;
            if (isRollUp) Util::bringWindowToTop(hwnd, this->hWnd());
            else {
                if (auto normal = rotateNormalWindowInGroup(groupWindowOrder, hwnd, false)) { ShowWindow(normal, SW_SHOWMINNOACTIVE); hwnd = normal; nextFocus = hwnd; }
                if (auto normal = rotateNormalWindowInGroup(groupWindowOrder, hwnd, false)) nextFocus = normal;
            }
            notifyForegroundChanged(nextFocus, Inner);
            showLabelForItem(item, Util::getWindowTitle(nextFocus));
            return true;
        }
    }
    return false;
}

void Widget::rotateTaskbarWindowInGroup(const QString& exePath, bool forward, int windows) {
    if (exePath.isEmpty()) return;
    if (!windows) return;
    static QString lastPath;
    static HWND lastHwnd = nullptr;
    if (lastPath != exePath) { lastPath = exePath; groupWindowOrder.clear(); }
    if (groupWindowOrder.isEmpty()) { groupWindowOrder = buildGroupWindowOrder(exePath); lastHwnd = nullptr; }
    if (groupWindowOrder.isEmpty()) return;
    static bool isLastForward = true;
    HWND hwnd = nullptr;
    if (!lastHwnd) {
        hwnd = groupWindowOrder.first();
        if (forward && hwnd == GetForegroundWindow()) hwnd = rotateWindowInGroup(groupWindowOrder, hwnd, true);
    } else {
        if (isLastForward == forward) hwnd = rotateWindowInGroup(groupWindowOrder, lastHwnd, forward);
        else hwnd = lastHwnd;
    }
    isLastForward = forward;
    if (forward) {
        static auto mouseEvent = [](DWORD flag) { mouse_event(flag, 0, 0, 0, 0); };
        if (windows == 1) {
            if (hwnd != GetForegroundWindow() || IsIconic(hwnd)) { mouseEvent(MOUSEEVENTF_LEFTDOWN); mouseEvent(MOUSEEVENTF_LEFTUP); qApp->processEvents(); }
        } else {
            if (HWND thumbnail = Util::getCurrentTaskListThumbnailWnd(); IsWindowVisible(thumbnail)) {
                mouseEvent(MOUSEEVENTF_LEFTDOWN);
                QTimer::singleShot(20, this, [hwnd]() { Util::switchToWindow(hwnd, true); });
            } else Util::switchToWindow(hwnd, true);
            static QTimer* timer = [this]() {
                auto* timer = new QTimer; timer->setSingleShot(true); timer->setInterval(200);
                timer->callOnTimeout(this, [this]() {
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    QTimer::singleShot(100, this, []() {
                        if (HWND thumbnail = Util::getCurrentTaskListThumbnailWnd(); IsWindowVisible(thumbnail)) {
                            if (HWND taskbar = FindWindow(L"Shell_TrayWnd", nullptr)) Util::switchToWindow(taskbar, true);
                        }
                    });
                });
                return timer;
            }();
            timer->stop(); timer->start();
        }
    } else {
        if (auto normal = rotateNormalWindowInGroup(groupWindowOrder, hwnd, false)) { ShowWindow(normal, SW_MINIMIZE); }
    }
    lastHwnd = hwnd;
}

HWND Widget::rotateWindowInGroup(const QList<HWND>& windows, HWND current, bool forward) {
    const auto N = windows.size();
    if (N == 1) return windows.first();
    for (int i = 0; i < N; i++) {
        if (windows.at(i) == current) {
            auto next_i = forward ? (i + 1) : (i - 1);
            return windows.at((next_i + N) % N);
        }
    }
    return nullptr;
}

HWND Widget::rotateNormalWindowInGroup(const QList<HWND>& windows, HWND current, bool forward) {
    for (int i = 0; IsIconic(current) && i < windows.size(); i++) current = rotateWindowInGroup(windows, current, forward);
    return IsIconic(current) ? nullptr : current;
}

void Widget::clearGroupWindowOrder() { groupWindowOrder.clear(); }
