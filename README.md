# Cloudy — Claude Desktop G-Sync Flicker Fix Launcher

**Cloudy** is an ultra-lightweight native Windows launcher (~24 KB, **zero dependencies**, **0% CPU in background**) that permanently fixes the screen flickering issue in **Claude Desktop** on **G-Sync / FreeSync** monitors.

---

## The Problem

On Windows 10 and 11, the Electron-based **Claude Desktop** app frequently triggers severe display flickering on G-Sync / FreeSync / VRR monitors unless launched with the `--disable-gpu-compositing` flag:

```cmd
cmd.exe /c start "" "C:\Program Files\WindowsApps\Claude_1.10628.2.0_x64__pzs8sxrjxfjjc\app\claude.exe" --disable-gpu-compositing
```

However, because Claude Desktop is distributed as an MSIX package inside `C:\Program Files\WindowsApps\`, **every time Claude updates, the versioned folder name changes** (e.g., `Claude_1.10628.2.0_x64__pzs8sxrjxfjjc` → `Claude_1.10700.0.0_x64__pzs8sxrjxfjjc`), breaking any manual desktop or taskbar shortcut.

On top of that, when Claude **auto-updates while it is open**, the updater restarts it **without** `--disable-gpu-compositing`, so the flicker comes back until you close Claude and launch it again with the flag.

---

## The Solution

**Cloudy (`Cloudy.exe`)** acts as a permanent, zero-overhead launcher:

- **Update-proof**: Automatically detects the latest installed version of Claude Desktop in **< 1 ms** every time you click it.
- **No Administrator privileges required**: Standard Windows users cannot list `C:\Program Files\WindowsApps` directly. Cloudy queries the Windows `AppModel\Repository\Packages` registry directly (`HKCU` / `HKLM`) to locate the exact package path without elevation, and also supports standard `%LOCALAPPDATA%\AnthropicClaude` installations.
- **Zero console window flash**: Compiled as a native Win32 GUI executable (`SUBSYSTEM:WINDOWS`) that launches `claude.exe` directly with `--disable-gpu-compositing`.
- **Survives auto-updates (guardian)**: After launching Claude, Cloudy stays in the background and simply waits on Claude's process (**0% CPU**, negligible RAM). If Claude is restarted without the flag — typically by the auto-updater, but also by "launch at login" or the Start menu — Cloudy closes that instance and relaunches the new version with `--disable-gpu-compositing`. When you quit Claude, Cloudy waits 3 minutes for a possible update restart and then exits. Use `--no-watch` to exit immediately after launch instead.
- **Fixes an already-running instance**: If Claude is already running without the flag when you click Cloudy, it is restarted with the flag (Electron's single-instance lock would otherwise just bring the flickering window to the front).

---

## Quick Start

1. Download **`Cloudy.exe`** from the [**Releases**](https://github.com/SpheralComa441/Cloudy/releases/latest) page.
2. Double-click **`Cloudy.exe`** to launch Claude Desktop (or pin it to your Taskbar / Start Menu / Desktop).
3. *(Optional)* Run [`Crea-Shortcut-Desktop.bat`](Crea-Shortcut-Desktop.bat) to automatically create a `Claude.lnk` shortcut on your Desktop. The shortcut uses the icon embedded in `Cloudy.exe`, because the icon inside the versioned `WindowsApps` folder disappears after every Claude update.

Whenever Claude updates in the future, you don't need to edit anything—just launch **Cloudy** as usual.

---

## Command-Line Options

| Command | Description |
| :--- | :--- |
| `Cloudy.exe` | *(Default)* Finds the latest installed version of Claude Desktop, launches it with `--disable-gpu-compositing` (restarting it first if it is already running without the flag) and keeps it on the flag across auto-updates. |
| `Cloudy.exe --status` | Displays a dialog showing the detected Claude version, full executable path, launch arguments, whether the running instance has the flag, and whether the guardian is active. |
| `Cloudy.exe --restart` | Terminates every running Claude Desktop process (including tray/background ones) and relaunches Claude cleanly with `--disable-gpu-compositing`. The Claude Code CLI (also named `claude.exe`) is never touched. |
| `Cloudy.exe --no-watch` | Launches Claude and exits immediately, without the update guardian. |

Any additional flags passed to `Cloudy.exe` are automatically forwarded (correctly quoted) to `claude.exe`.

> **Tip:** if Claude Desktop is set to start at Windows login, it starts without the flag. Either disable that option in Claude and put a Cloudy shortcut in `shell:startup`, or just click Cloudy once after login: the flickering instance is restarted with the flag.

---

## Building from Source

Cloudy is written in freestanding Win32 C ([`src/cloudy.c`](src/cloudy.c)) with no C runtime (`-nostdlib`) and links only against core Win32 system DLLs (`kernel32.dll`, `advapi32.dll`, `user32.dll`, `shell32.dll`, `ntdll.dll`). An equivalent single-file C# implementation is also provided in [`src/Cloudy.cs`](src/Cloudy.cs).

- **On Windows (zero tools required)**: Double-click [`Build-Local.bat`](Build-Local.bat) to compile using the `.NET Framework` `csc.exe` compiler preinstalled on every Windows 10/11 machine.
- **Cross-compile with Clang/LLD**: Run [`./scripts/build.sh`](scripts/build.sh).
- **Releases**: pushing a `v*` tag (or running the workflow manually with a tag) builds `Cloudy.exe` from source with GitHub Actions ([`release.yml`](.github/workflows/release.yml)) and publishes it with its SHA-256 checksum. The same build also runs on every push to `main` and every pull request.
