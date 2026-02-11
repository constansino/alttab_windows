#ifndef ALTTAB_WINDOWS_CONFIGMANAGER_H
#define ALTTAB_WINDOWS_CONFIGMANAGER_H

#include <QSettings>
#include <QApplication>
#include "ConfigManagerBase.h"

// ע�⣺���ڴ���ʹ�õ��࣬header-only ģʽ�ᵼ�±���ʱ�����
#define cfg ConfigManager::instance()

enum DisplayMonitor {
    PrimaryMonitor, // 0 ����ʾ��
    MouseMonitor, // 1 �������
    EnumCount // Just for count
};

class ConfigManager : public ConfigManagerBase {
    inline static const QString FileName = "config.ini";

public:
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    static ConfigManager& instance() {
        static const auto filePath = QApplication::applicationDirPath() + "/" + FileName;
        static ConfigManager instance{filePath}; // multiple threads safe
        return instance;
    }

public:
    DisplayMonitor getDisplayMonitor() {
        auto monitor = get("DisplayMonitor", DisplayMonitor::MouseMonitor).toInt();
        if (monitor < 0 || monitor >= DisplayMonitor::EnumCount) {
            qWarning() << "Invalid DisplayMonitor enum" << monitor;
            monitor = DisplayMonitor::MouseMonitor;
        }
        return static_cast<DisplayMonitor>(monitor);
    }

    void setDisplayMonitor(DisplayMonitor monitor) {
        set("DisplayMonitor", monitor);
    }

    bool getHoldMode() {
        return get("HoldMode", false).toBool();
    }

    void setHoldMode(bool on) {
        set("HoldMode", on);
    }

    bool getMouseWarp() {
        return get("MouseWarp", true).toBool();
    }

    void setMouseWarp(bool on) {
        set("MouseWarp", on);
    }

    // Pinned Apps: ExeName -> Index (0-based)
    QMap<QString, int> getPinnedApps() {
        auto map = get("PinnedApps").toMap();
        QMap<QString, int> result;
        for (auto it = map.begin(); it != map.end(); ++it) {
            result.insert(it.key(), it.value().toInt());
        }
        return result;
    }

    void setPinnedApp(const QString& exeName, int index) {
        auto map = get("PinnedApps").toMap();
        if (index < 0) map.remove(exeName);
        else map.insert(exeName, index);
        set("PinnedApps", map);
    }

    // Shortcuts: ExeName -> Key (e.g. "F1", "Ctrl+1")
    QMap<QString, QString> getAppShortcuts() {
        auto map = get("AppShortcuts").toMap();
        QMap<QString, QString> result;
        for (auto it = map.begin(); it != map.end(); ++it) {
            result.insert(it.key(), it.value().toString());
        }
        return result;
    }

    void setAppShortcut(const QString& exeName, const QString& key) {
        auto map = get("AppShortcuts").toMap();
        if (key.isEmpty()) map.remove(exeName);
        else map.insert(exeName, key);
        set("AppShortcuts", map);
    }

    // Global Shortcuts: ExeName -> Bool
    bool isShortcutGlobal(const QString& exeName) {
        auto map = get("GlobalShortcuts").toMap();
        return map.value(exeName, false).toBool();
    }

    void setShortcutGlobal(const QString& exeName, bool global) {
        auto map = get("GlobalShortcuts").toMap();
        map.insert(exeName, global);
        set("GlobalShortcuts", map);
    }

    enum HotCornerPos {
        None = 0,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };

    HotCornerPos getHotCornerPosition() {
        return static_cast<HotCornerPos>(get("HotCornerPosition", 0).toInt());
    }

    void setHotCornerPosition(HotCornerPos pos) {
        set("HotCornerPosition", static_cast<int>(pos));
    }

    QString getHotCornerApp() {
        return get("HotCornerApp", "").toString();
    }

    void setHotCornerApp(const QString& exeName) {
        set("HotCornerApp", exeName);
    }

private:
    explicit ConfigManager(const QString& filename) : ConfigManagerBase(filename) {}
};


#endif // ALTTAB_WINDOWS_CONFIGMANAGER_H


