# Cloudy — Claude Desktop G-Sync Flicker Fix Launcher

**Cloudy** is an ultra-lightweight native Windows launcher (~20 KB, **zero dependencies**, **0 MB background RAM**) that permanently fixes the screen flickering issue in **Claude Desktop** on **G-Sync / FreeSync** monitors.

---

## The Problem

On Windows 10 and 11, the Electron-based **Claude Desktop** app frequently triggers severe display flickering on G-Sync / FreeSync / VRR monitors unless launched with the `--disable-gpu-compositing` flag:

```cmd
cmd.exe /c start "" "C:\Program Files\WindowsApps\Claude_1.10628.2.0_x64__pzs8sxrjxfjjc\app\claude.exe" --disable-gpu-compositing
```

However, because Claude Desktop is distributed as an MSIX package inside `C:\Program Files\WindowsApps\`, **every time Claude updates, the versioned folder name changes** (e.g., `Claude_1.10628.2.0_x64__pzs8sxrjxfjjc` → `Claude_1.10700.0.0_x64__pzs8sxrjxfjjc`), breaking any manual desktop or taskbar shortcut.

---

## The Solution

**Cloudy (`Cloudy.exe`)** acts as a permanent, zero-overhead launcher:

- **Update-proof**: Automatically detects the latest installed version of Claude Desktop in **< 1 ms** every time you click it.
- **No Administrator privileges required**: Standard Windows users cannot list `C:\Program Files\WindowsApps` directly. Cloudy queries the Windows `AppModel\Repository\Packages` registry directly (`HKCU` / `HKLM`) to locate the exact package path without elevation, and also supports standard `%LOCALAPPDATA%\AnthropicClaude` installations.
- **Zero console window flash**: Compiled as a native Win32 GUI executable (`SUBSYSTEM:WINDOWS`) that spawns Claude with `--disable-gpu-compositing` using `CREATE_NO_WINDOW`.
- **Zero background footprint**: Exits immediately after launching Claude (**0 MB RAM** used in the background).

---

## Quick Start

1. Download **`Cloudy.exe`** from the [**Releases**](https://github.com/SpheralComa441/Cloudy/releases/latest) page.
2. Double-click **`Cloudy.exe`** to launch Claude Desktop (or pin it to your Taskbar / Start Menu / Desktop).
3. *(Optional)* Run [`Crea-Shortcut-Desktop.bat`](Crea-Shortcut-Desktop.bat) to automatically create a `Claude.lnk` shortcut on your Desktop using the official icon extracted from your installed `claude.exe`.

Whenever Claude updates in the future, you don't need to edit anything—just launch **Cloudy** as usual.

---

## Command-Line Options

| Command | Description |
| :--- | :--- |
| `Cloudy.exe` | *(Default)* Finds the latest installed version of Claude Desktop and launches it with `--disable-gpu-compositing`. |
| `Cloudy.exe --status` | Displays a dialog showing the detected Claude version, full executable path, and launch arguments. |
| `Cloudy.exe --restart` | Terminates any running background/tray `claude.exe` processes and relaunches Claude cleanly with `--disable-gpu-compositing`. |

Any additional flags passed to `Cloudy.exe` are automatically forwarded to `claude.exe`.

---

## Building from Source

Cloudy is written in freestanding Win32 C ([`src/cloudy.c`](src/cloudy.c)) with no C runtime (`-nostdlib`) and links only against core Win32 system DLLs (`kernel32.dll`, `advapi32.dll`, `user32.dll`, `shell32.dll`). An equivalent single-file C# implementation is also provided in [`src/Cloudy.cs`](src/Cloudy.cs).

- **On Windows (zero tools required)**: Double-click [`Build-Local.bat`](Build-Local.bat) to compile using the `.NET Framework` `csc.exe` compiler preinstalled on every Windows 10/11 machine.
- **Cross-compile with Clang/LLD**: Run [`./scripts/build.sh`](scripts/build.sh).
