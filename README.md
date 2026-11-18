# Cloudy — Claude Desktop Flicker Workaround for Windows

**Cloudy** is a tiny native Windows launcher (~28 KB, **zero dependencies**, **0% CPU in background**) that starts **Claude Desktop** with the Chromium switches that stop the screen flickering for many users on **G-Sync / FreeSync / VRR** monitors, and keeps them on across Claude's auto-updates.

> **It is a workaround, not a guaranteed fix.** Claude Desktop flickering has more than one cause. Cloudy only changes how Claude's window is composited; if your flicker comes from somewhere else, Cloudy will not help. See [Will Cloudy work for me?](#will-cloudy-work-for-me) before you start.

---

## The Problem

On Windows 10 and 11, the Electron-based **Claude Desktop** app can flicker badly on G-Sync / FreeSync / VRR monitors. For many users, launching it with the `--disable-gpu-compositing` switch makes the flicker go away:

```cmd
cmd.exe /c start "" "C:\Program Files\WindowsApps\Claude_1.10628.2.0_x64__pzs8sxrjxfjjc\app\claude.exe" --disable-gpu-compositing
```

On some NVIDIA RTX 50 series setups that switch was not enough, and `--disable-direct-composition` was needed instead.

Doing this by hand is fragile:

- Claude Desktop is an MSIX package inside `C:\Program Files\WindowsApps\`, and **every update changes the versioned folder name** (e.g., `Claude_1.10628.2.0_x64__pzs8sxrjxfjjc` → `Claude_1.10700.0.0_x64__pzs8sxrjxfjjc`), which breaks any manual shortcut.
- When Claude **auto-updates while it is open**, the updater restarts it **without** the switch, so the flicker comes back until you close Claude and launch it again with the switch.

---

## Will Cloudy work for me?

Cloudy can only help if the flicker is caused by how Claude's window is composited (GPU compositing or DirectComposition) on a variable-refresh-rate display. It does **not** help when the cause is elsewhere. For example, a report of G-Sync flicker where frames arrive in bursts had already ruled out the GPU compositing conflict; launch switches cannot fix that case.

The quickest way to find out:

1. Run `Cloudy.exe` (default mode `gpu`).
2. If it still flickers, switch to `dcomp`, then to `both` (see [Choosing the fix mode](#choosing-the-fix-mode)).
3. If none of the three modes helps, Cloudy is not the right tool for your setup. Please [open an issue](https://github.com/SpheralComa441/Cloudy/issues) with your configuration so the table below stays useful for others.

### Compatibility

| GPU | Monitor / VRR | Mode that works | Result | Notes |
| :--- | :--- | :--- | :---: | :--- |
| NVIDIA GeForce RTX 50 series (some cards) | — | `dcomp` | ✅ | `gpu` alone was not enough; `--disable-direct-composition` was needed. |
| — | G-Sync, frames delivered in bursts | none | ❌ | GPU compositing conflict ruled out by the reporter: not a case Cloudy can fix. |

To add your setup, open an issue with: GPU and driver version, monitor model with refresh rate and VRR technology (G-Sync / G-Sync Compatible / FreeSync), Windows version, which Cloudy mode works (or none), and the text of `Cloudy.exe --status`.

---

## Quick Start

1. Download **`Cloudy.exe`** from the [**Releases**](https://github.com/SpheralComa441/Cloudy/releases/latest) page.
2. Double-click **`Cloudy.exe`** to launch Claude Desktop (or pin it to your Taskbar / Start Menu / Desktop).
3. If Claude still flickers, try the other [fix modes](#choosing-the-fix-mode).
4. *(Optional)* Run [`Crea-Shortcut-Desktop.bat`](Crea-Shortcut-Desktop.bat) to create a `Claude.lnk` shortcut on your Desktop. The shortcut uses the icon embedded in `Cloudy.exe`, because the icon inside the versioned `WindowsApps` folder disappears after every Claude update.

Whenever Claude updates, you don't need to edit anything: just launch **Cloudy** as usual.

---

## Choosing the fix mode

| Mode | Switches passed to Claude | When to use it |
| :--- | :--- | :--- |
| `gpu` *(default)* | `--disable-gpu-compositing` | First thing to try. |
| `dcomp` | `--disable-direct-composition` | When `gpu` does not stop the flicker (reported on some RTX 50 series cards). |
| `both` | `--disable-gpu-compositing --disable-direct-composition` | When neither switch alone is enough. |

To change it without touching your shortcuts, create a text file named **`Cloudy.ini`** in the same folder as `Cloudy.exe`:

```ini
; Cloudy.ini — fix mode: gpu (default), dcomp or both
mode=dcomp
```

The mode can also be set on the command line, which overrides `Cloudy.ini`: `Cloudy.exe --mode=both`, or `Cloudy.exe --dcomp` as a shortcut for `--mode=dcomp`. An invalid value shows a warning and Cloudy keeps going with the previous mode.

After changing the mode, launch Cloudy again: a running Claude that lacks the new switches is restarted with them. `Cloudy.exe --status` shows the active mode, where it came from, and the path of the `Cloudy.ini` file Cloudy reads.

---

## How it works

- **Update-proof**: Finds the latest installed version of Claude Desktop every time you click it.
- **No Administrator privileges required**: Standard Windows users cannot list `C:\Program Files\WindowsApps` directly. Cloudy reads the Windows `AppModel\Repository\Packages` registry (`HKCU` / `HKLM`) to locate the package path without elevation, and also supports standard `%LOCALAPPDATA%\AnthropicClaude` installations.
- **No console window**: A native Win32 GUI executable (`SUBSYSTEM:WINDOWS`) that launches `claude.exe` directly with the switches of the selected mode.
- **Survives auto-updates (guardian)**: After launching Claude, Cloudy stays in the background and simply waits on Claude's process (**0% CPU**, negligible RAM). If Claude is restarted without the switches (typically by the auto-updater, but also by "launch at login" or the Start menu), Cloudy closes that instance and relaunches the new version with them. When you quit Claude, Cloudy waits 3 minutes for a possible update restart and then exits. Use `--no-watch` to exit immediately after launch instead.
- **Fixes an already-running instance**: If Claude is already running without the switches when you click Cloudy, it is restarted with them (Electron's single-instance lock would otherwise just bring the flickering window to the front).

> **Tip:** if Claude Desktop is set to start at Windows login, it starts without the switches. Either disable that option in Claude and put a Cloudy shortcut in `shell:startup`, or just click Cloudy once after login: the flickering instance is restarted with the switches.

---

## Command-Line Options

| Command | Description |
| :--- | :--- |
| `Cloudy.exe` | *(Default)* Finds the latest installed version of Claude Desktop, launches it with the switches of the selected mode (restarting it first if it is already running without them) and keeps them on across auto-updates. |
| `Cloudy.exe --mode=gpu\|dcomp\|both` | Selects the [fix mode](#choosing-the-fix-mode) for this launch, overriding `Cloudy.ini`. |
| `Cloudy.exe --dcomp` | Same as `--mode=dcomp`. |
| `Cloudy.exe --status` | Displays a dialog showing the detected Claude version and path, the active mode and its source, the launch arguments, whether the running instance has the switches, and whether the guardian is active. |
| `Cloudy.exe --restart` | Terminates every running Claude Desktop process (including tray/background ones) and relaunches Claude cleanly with the switches. The Claude Code CLI (also named `claude.exe`) is never touched. |
| `Cloudy.exe --no-watch` | Launches Claude and exits immediately, without the update guardian. |

Any other arguments passed to `Cloudy.exe` are forwarded (correctly quoted) to `claude.exe`.

---

## Building from Source

There is a single implementation: [`src/cloudy.c`](src/cloudy.c), freestanding Win32 C with no C runtime (`-nostdlib`), linking only against core Windows DLLs (`kernel32.dll`, `advapi32.dll`, `user32.dll`, `shell32.dll`, `ntdll.dll`). It is exactly what the releases ship. (Up to v1.1.0 there was also a C# copy built by `Build-Local.bat`; it was removed so that a local build and the release are the same program.)

- **Linux or WSL (Ubuntu 24.04)**: `sudo apt install clang lld llvm`, then run [`./scripts/build.sh`](scripts/build.sh). This is the same toolchain the release workflow uses.
- **Without building**: every push to `main` and every pull request is built by GitHub Actions; the `Cloudy` artifact of a workflow run contains `Cloudy.exe` and its SHA-256.
- **Releases**: pushing a `v*` tag (or running the workflow manually with a tag) builds `Cloudy.exe` from source with GitHub Actions ([`release.yml`](.github/workflows/release.yml)) and publishes it with its SHA-256 checksum.
