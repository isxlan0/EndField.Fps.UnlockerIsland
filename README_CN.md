# EndField.Fps.UnlockerIsland

[English](./README.md)

EndField 的 Windows 启动器 + DLL 注入工具。启动游戏，注入 `UnlockerIsland.dll`，并通过共享内存控制目标帧率 (FPS)。

## 功能

- 可使用 `-popupwindow` 和自定义参数启动游戏

- 通过 LoadLibrary 注入 `UnlockerIsland.dll`

- 可在启动器界面解锁帧率并设置目标帧率

- 启动器和注入的 DLL 之间共享内存

## 系统要求

- Windows 10/11 (x64)

- Visual Studio 2026

- C++20 (`/std:c++20`)

## 构建

1. 在 Visual Studio 2026 中打开 `EndField.Fps.UnlockerIsland.slnx`。

2. 选择 `Release | x64`（或 Debug）。

3. 构建解决方案。

## 运行

1. 运行 `EndField.Launcher.exe`。

2. 选择游戏可执行文件。

3. （可选）启用注入并设置帧率。

4. 点击**启动游戏**。

## 注意事项

- 注入和启动游戏需要以管理员身份运行。

- `UnlockerIsland.dll` 应该位于启动器所在文件夹或构建输出文件夹中。

## 协议

本项目基于 [MIT 协议](./LICENSE) 开源发布。  
允许自由使用、修改与发布，需保留原始作者署名。

## 声明

本工具为非官方第三方插件。  
使用可能违反游戏的用户协议，存在封号风险。  
作者不对使用本工具产生的任何后果承担责任。

## 反馈

如需反馈 Bug 或提交建议，请在 [Issues 页面](https://github.com/isxlan0/Genshin.Fps.UnlockerIsland/issues) 提出。

---

*本项目仅供学习与研究使用，禁止用于商业用途。*

