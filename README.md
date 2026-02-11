# alttab_windows

![GitHub release (latest by date)](https://github.com/constansino/alttab_windows/releases)
![Language](https://img.shields.io/badge/language-C%2B%2B-239120)
![OS](https://img.shields.io/badge/OS-Windows-0078D4)

一个 MacOS 风格的 Alt-Tab 窗口/应用切换器，专为 Windows 打造。强调“快、稳、准”，并支持多显示器与系统托盘控制。

![ui](img/ui.png)

## 功能特性

- 智能应用切换 (Alt+Tab)
- 目标锁定 (Target-Lock)，默认选中上一个活跃应用
- Hold / Toggle 模式，按住切换或点按呼出
- 组内窗口切换 (Alt+`)，在同一应用多窗口间快速轮换
- 任务栏滚轮增强，在任务栏图标上滚动滚轮切换/最小化窗口
- 全局快捷键，可将任意应用绑定快捷键并支持“后台一键切换/最小化”
- 右键菜单，支持关闭窗口、强制结束、固定位置、录制快捷键
- 鼠标吸附 (Mouse Warp)，呼出时自动移动到当前选中项并支持自动归位
- 多显示器支持，可选择“主屏”或“鼠标所在屏”居中显示
- 系统托盘控制：更新检查、设置入口、自启动、显示器选择等

## 快速开始

1. 运行 `alttab_windows.exe`（建议管理员权限以获得更完整的窗口列表与控制能力）。
2. 使用默认热键进行切换。

常用快捷键

- Alt+Tab：呼出并向前切换
- Alt+Shift+Tab：反向切换
- Alt+`：组内切换
- Enter / Space：确认选择
- Esc：取消
- 上/下方向键：在当前组内进行轮换

## 托盘菜单

- Check for Updates：检查 GitHub Releases 更新
- Settings：打开 `config.ini` 并自动热重载
- Start with Windows：开机自启动（管理员时优先使用计划任务）
- Hold Mode：切换按住/点按模式
- Auto Mouse Warp：开启/关闭鼠标吸附
- Display Monitor：主屏或鼠标所在屏

## 配置文件

配置文件位于程序目录下 `config.ini`，修改后会自动重载。

```ini
[General]
HoldMode=true
MouseWarp=true
DisplayMonitor=1   # 0=主屏 1=鼠标所在屏

[label]
font_family = "Microsoft YaHei UI"
font_size = 10
```

快捷键/固定项由程序通过配置自动维护，一般无需手动编辑。

## 构建说明

本项目使用 CMake + Qt6 (MSVC) 构建。

前置条件

- Visual Studio 2022 Build Tools (MSVC v143)
- Qt 6.8.1 (msvc2022_64)
- CMake >= 3.29

构建步骤

1. 确认 `CMakeLists.txt` 中的 `CMAKE_PREFIX_PATH` 指向本机 Qt 安装路径。
2. 生成并编译：

```powershell
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
```

## 打包说明

- Release 输出位于 `build/Release/alttab_windows.exe`
- 便携包可用 `windeployqt` 或直接将 exe 与 Qt 运行库放入同目录（见 `alttab_windows_v0.5.0_portable/`）
- 更新检查依赖 GitHub Releases 的 zip 资产

## 权限说明

- 以管理员权限运行可更完整地枚举/控制高权限窗口（如任务管理器）。

## 参考与致谢

- window-switcher: https://github.com/sigoden/window-switcher
- cmdtab: https://github.com/stianhoiland/cmdtab
