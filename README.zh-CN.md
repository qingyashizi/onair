<div align="center">
  <img src="app.ico" width="64" height="64" alt="OnAir 图标">
  <h1>OnAir</h1>
  <p>轻量级 Windows 托盘工具，当麦克风或摄像头被使用时，在屏幕右下角显示动态提示浮层。</p>

  [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
  [![Platform: Windows](https://img.shields.io/badge/platform-Windows%2010%2B-0078d7.svg)](https://github.com/qingyashizi/onair/releases)
  [![Release](https://img.shields.io/github/v/release/qingyashizi/onair)](https://github.com/qingyashizi/onair/releases/latest)
  [![Size](https://img.shields.io/badge/size-~41%20KB-brightgreen)](https://github.com/qingyashizi/onair/releases/latest)

  [English Documentation](README.md)
</div>

---

## 这是什么？

OnAir 是一款极简的 Windows 系统托盘工具，实时监测麦克风和摄像头的使用状态。当某个应用开始使用你的麦克风或摄像头时，屏幕右下角会出现一个平滑淡入的动画浮层——告诉你是哪个应用正在监听或拍摄。

设备释放后，浮层自动淡出消失。无需打开任何菜单或面板，安静地在后台运行。

## 截图

| 暗色模式 | 亮色模式 |
|----------|----------|
| ![暗色模式 — 麦克风和摄像头同时活跃](dark%20two.png) | ![亮色模式 — 麦克风和摄像头同时活跃](ligth%20two.png) |

## 功能特性

- **动画浮层** — 使用 GDI+ 渲染，平滑淡入/淡出，显示在屏幕右下角
- **显示应用名称** — 展示当前正在使用设备的应用程序名称
- **麦克风 + 摄像头同时显示** — 两个设备同时活跃时左右并排显示
- **麦克风动画** — 麦克风图标内部脉冲填充动画
- **摄像头动画** — 摄像头图标周围旋转四角取景框动画
- **暗色 / 亮色主题** — 跟随 Windows 系统主题，或通过托盘菜单手动切换
- **中文 / 英文界面** — 跟随系统语言，或手动切换
- **开机自启动** — 托盘菜单一键开关（写入 HKCU Run 注册表键）
- **OSD 预览模式** — 右键托盘 → 预览 OSD
- **DPI 自适应** — Per-Monitor DPI V2，4K/HiDPI 屏幕显示清晰
- **~41 KB** — 单个 `.exe`，无需安装，无运行时依赖

## 系统要求

- Windows 10 及以上版本（x64）
- 麦克风/摄像头监测需要 Windows 10 1903 及以上（CapabilityAccessManager 注册表）

## 安装方式

1. 从 [**最新发布页面**](https://github.com/qingyashizi/onair/releases/latest) 下载 `onair.exe`
2. 放置到任意目录（例如 `C:\Tools\onair.exe`）
3. 双击运行 — **ON** 图标出现在系统托盘区域
4. 可选：右键托盘图标 → **开机启动**，开启自动启动

**卸载方式：** 右键托盘图标 → **退出 OnAir**，然后删除 `.exe` 文件即可。如果已开启开机启动，请先在托盘菜单中关闭，再删除文件。

## SmartScreen 安全警告说明

首次下载运行 `onair.exe` 时，Windows SmartScreen 或浏览器可能会弹出安全警告。这是新发布软件的**正常现象**，并不代表文件有危险。

**原因：** Windows SmartScreen 会根据签名程序的累计下载量逐步建立信誉。新发布的应用——即使拥有有效的代码签名证书——在积累足够下载量之前初始信誉为零，因此会触发警告。

**文件已签名：** `onair.exe` 使用 **Certum** 颁发的 Authenticode 代码签名证书（签名者：*Fangmeng Liu*）进行了签名。你可以自行验证：

```powershell
Get-AuthenticodeSignature -FilePath "路径\onair.exe" | Select-Object Status, SignerCertificate
```

**代码完全开源：** 本仓库包含所有源代码，欢迎运行前自行审阅，也可以从源码自行构建。

**跳过警告的操作方法：**

*浏览器下载警告（Chrome / Edge）：*
1. 点击下载项旁的三点菜单（⋮）→ **保留** → **仍然保留**

*Windows SmartScreen 弹窗（「Windows 已保护你的电脑」）：*
1. 点击 **「更多信息」**
2. 点击 **「仍要运行」**

## 配置文件

OnAir 的配置保存在 `onair.ini` 文件中，与 `.exe` 位于同一目录。首次通过托盘菜单更改设置时自动创建。

| 键名 | 取值 | 说明 |
|------|------|------|
| `ThemeMode` | `0` — 跟随系统 *（默认）* | 跟随 Windows 暗色/亮色模式 |
| | `1` — 暗色模式 | 始终使用暗色主题 |
| | `2` — 亮色模式 | 始终使用亮色主题 |
| `LangMode` | `0` — 跟随系统 *（默认）* | 跟随 Windows 显示语言 |
| | `1` — 中文 | 始终使用中文界面 |
| | `2` — English | 始终使用英文界面 |

在托盘菜单中更改设置后立即保存。也可用任意文本编辑器直接编辑 `onair.ini`。

## 从源码构建

**依赖：**
- CMake 3.10+
- MSVC（Visual Studio 2019+）**或** MinGW-w64
- 无外部库依赖 — 仅使用 Windows 内置 API（GDI+、Shell32、User32、Advapi32）

```bash
git clone https://github.com/qingyashizi/onair.git
cd onair
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

编译后的二进制文件位于 `build\Release\onair.exe`（MSVC）或 `build\onair.exe`（MinGW）。

## 开源协议

MIT — 详见 [LICENSE](LICENSE)
