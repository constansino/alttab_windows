#ifndef ALTTAB_WINDOWS_SYSTEMTRAY_H
#define ALTTAB_WINDOWS_SYSTEMTRAY_H

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QActionGroup>
#include "Startup.h"
#include "ConfigManager.h"
#include "UpdateDialog.h"

#define sysTray SystemTray::instance()

class SystemTray : public QSystemTrayIcon {
public:
    SystemTray(const SystemTray&) = delete;
    SystemTray& operator=(const SystemTray&) = delete;

    static SystemTray& instance() {
        static SystemTray instance;
        return instance;
    }

private:
    explicit SystemTray(QWidget* parent = nullptr) : QSystemTrayIcon(parent) {
        setIcon(QIcon(":/img/icon.ico"));
        setMenu(parent);
        setToolTip(IsUserAnAdmin() ? "alttab_windows (admin)" : "alttab_windows");
    }

    void setMenu(QWidget* parent = nullptr) {
        auto* menu = new QMenu(parent);
        menu->setStyleSheet("QMenu{"
            "background-color:rgb(45,45,45);"
            "color:rgb(220,220,220);"
            "border:1px solid black;"
            "}"
            "QMenu:selected{ background-color:rgb(60,60,60); }");

        auto* act_update = new QAction("Check for Updates", menu);
        auto* act_settings = new QAction("Settings", menu);
        auto* act_startup = new QAction("Start with Windows", menu);
        auto* act_hold_mode = new QAction("Hold Mode", menu);
        auto* act_mouse_warp = new QAction("Auto Mouse Warp", menu);
        auto* menu_monitor = new QMenu("Display Monitor", menu);
        auto* act_quit = new QAction("Quit >", menu);

        connect(act_update, &QAction::triggered, this, [] {
            static auto* dlg = new UpdateDialog;
            dlg->show();
        });

        connect(act_settings, &QAction::triggered, this, [] {
            cfg.editConfigFile();
        });
        
        connect(&cfg, &ConfigManager::configEdited, this, [this] {
            this->showMessage("Config Edited", "auto reloaded");
        });

        act_hold_mode->setCheckable(true);
        connect(act_hold_mode, &QAction::triggered, this, [this](bool checked) {
            cfg.setHoldMode(checked);
            this->showMessage("Hold Mode Changed", checked ? "ON (Standard)" : "OFF (Toggle)");
        });
        connect(menu, &QMenu::aboutToShow, act_hold_mode, [act_hold_mode] {
            act_hold_mode->setChecked(cfg.getHoldMode());
        });

        act_mouse_warp->setCheckable(true);
        connect(act_mouse_warp, &QAction::triggered, this, [this](bool checked) {
            cfg.setMouseWarp(checked);
            this->showMessage("Mouse Warp Changed", checked ? "ON (Auto Move)" : "OFF");
        });
        connect(menu, &QMenu::aboutToShow, act_mouse_warp, [act_mouse_warp] {
            act_mouse_warp->setChecked(cfg.getMouseWarp());
        });

        act_startup->setCheckable(true);
        connect(act_startup, &QAction::triggered, this, [this](bool checked) {
            Startup::toggle();
            if (Startup::isOn() == checked)
                this->showMessage("auto Startup mode", checked ? "ON" : "OFF");
            else
                this->showMessage("Action Failed", "Failed to change Startup mode", Warning);
        });
        connect(menu, &QMenu::aboutToShow, act_startup, [act_startup] {
            act_startup->setChecked(Startup::isOn());
        });

        {
            auto* monitorGroup = new QActionGroup(menu_monitor);
            auto* actPrimary = monitorGroup->addAction("Primary Monitor");
            actPrimary->setData(PrimaryMonitor);
            actPrimary->setCheckable(true);
            auto* actMouse = monitorGroup->addAction("Mouse Monitor");
            actMouse->setData(MouseMonitor);
            actMouse->setCheckable(true);

            menu_monitor->addActions(monitorGroup->actions());

            connect(menu_monitor, &QMenu::aboutToShow, this, [monitorGroup] {
                monitorGroup->actions()[cfg.getDisplayMonitor()]->setChecked(true);
            });

            connect(monitorGroup, &QActionGroup::triggered, this, [this](QAction* act) {
                auto monitor = static_cast<DisplayMonitor>(act->data().toInt());
                cfg.setDisplayMonitor(monitor);
                this->showMessage("Display Monitor Changed", act->text());
            });
        }

        connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);

        menu->addAction(act_update);
        menu->addAction(act_settings);
        menu->addAction(act_startup);
        menu->addAction(act_hold_mode);
        menu->addAction(act_mouse_warp);
        menu->addMenu(menu_monitor);
        menu->addAction(act_quit);
        this->setContextMenu(menu);
    }
};

#endif // ALTTAB_WINDOWS_SYSTEMTRAY_H