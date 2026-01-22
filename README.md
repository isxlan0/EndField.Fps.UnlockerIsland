# EndField.Fps.UnlockerIsland

[简体中文](./README_CN.md)

A Windows launcher + DLL injection tool for EndField. It starts the game, injects `UnlockerIsland.dll`, and controls the target FPS through shared memory.

## Features
- Launch game with optional `-popupwindow` and custom arguments
- Inject `UnlockerIsland.dll` via LoadLibrary
- Unlock FPS and set target FPS from the launcher UI
- Shared memory control between launcher and injected DLL

## Requirements
- Windows 10/11 (x64)
- Visual Studio 2026
- C++20 (`/std:c++20`)

## Build
1. Open `EndField.Fps.UnlockerIsland.slnx` in Visual Studio 2026.
2. Select `Release | x64` (or Debug).
3. Build solution.

## Run
1. Launch `EndField.Launcher.exe`.
2. Select the game executable.
3. (Optional) Enable injection and set FPS.
4. Click **Launch Game**.

## Notes
- Running as Administrator is required for injection and launching.
- `UnlockerIsland.dll` should be located next to the launcher or in the build output folder.

## License

This project is released under the [MIT License](./LICENSE).

Free use, modification, and distribution are permitted, provided the original author's attribution is retained.

## Declaration

This tool is an unofficial third-party plugin.

Use may violate the game's user agreement and carries the risk of account banning.

The author assumes no responsibility for any consequences arising from the use of this tool.

## Feedback

To report bugs or submit suggestions, please do so on the [Issues page](https://github.com/isxlan0/Genshin.Fps.UnlockerIsland/issues).

---

*This project is for learning and research purposes only and is prohibited for commercial use.*