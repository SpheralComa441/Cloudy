// Cloudy - launcher for Claude Desktop with --disable-gpu-compositing (G-Sync flicker fix).
// C# equivalent of src/cloudy.c, compiled by Build-Local.bat with the .NET Framework csc.exe
// (C# 5 language level: no string interpolation, no expression-bodied members, no out var).
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;

namespace Cloudy
{
    internal static class Program
    {
        private const string GpuFlag = "--disable-gpu-compositing";
        private const string GuardianMutexName = @"Local\Cloudy.Guardian";
        private const int GuardPollMs = 2000;       // poll interval while Claude is NOT running
        private const int GuardGraceMs = 180000;    // after Claude exits, wait this long for an updater relaunch
        private const int GuardMaxRestarts = 3;     // consecutive fix attempts before giving up (avoid loops)
        private const int KillTimeoutMs = 5000;

        private sealed class Candidate
        {
            public string ExePath = "";
            public string AppDir = "";
            public Version ParsedVersion = new Version(0, 0, 0, 0);
        }

        private sealed class ClaudeProc
        {
            public int Pid;
            public int ParentPid;
            public bool IsMain;
        }

        private static Candidate _best;
        private static string _launchArgs = GpuFlag;

        [STAThread]
        private static int Main(string[] args)
        {
            bool showStatus = false;
            bool restartClaude = false;
            bool noWatch = false;
            var launchArgs = new StringBuilder(GpuFlag);

            foreach (string arg in args)
            {
                if (IsOption(arg, "--status", "/status"))
                {
                    showStatus = true;
                }
                else if (IsOption(arg, "--restart", "/restart"))
                {
                    restartClaude = true;
                }
                else if (IsOption(arg, "--no-watch", "/nowatch"))
                {
                    noWatch = true;
                }
                else if (!string.Equals(arg, GpuFlag, StringComparison.OrdinalIgnoreCase))
                {
                    launchArgs.Append(' ').Append(QuoteArg(arg));
                }
            }
            _launchArgs = launchArgs.ToString();

            FindLatestClaude();

            if (_best == null || string.IsNullOrEmpty(_best.ExePath))
            {
                MessageBox.Show(
                    "Impossibile trovare l'installazione di Claude Desktop.\n\n" +
                    "Percorsi controllati:\n" +
                    "• C:\\Program Files\\WindowsApps\\Claude_*\\app\\claude.exe\n" +
                    "• %LOCALAPPDATA%\\AnthropicClaude\\claude.exe\n\n" +
                    "Assicurati che l'app Claude Desktop sia installata.",
                    "Cloudy - Claude non trovato",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
                return 1;
            }

            if (showStatus)
            {
                ShowStatus();
                return 0;
            }

            // If Claude is already running without the flag (started by the auto-updater,
            // by "launch at login", from the Start menu...), Electron's single-instance lock
            // would just forward our launch to that flickering instance. Restart it instead.
            List<ClaudeProc> running = ScanClaudeProcesses();
            if (running.Count > 0 && (restartClaude || AnyMainWithoutFlag(running)))
            {
                KillDesktopClaude();
            }

            if (!LaunchClaude())
            {
                MessageBox.Show(
                    "Impossibile avviare Claude Desktop.\nVerifica i permessi dell'applicazione.",
                    "Cloudy - Errore di avvio",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
                return 1;
            }

            if (!noWatch)
            {
                RunGuardian();
            }
            return 0;
        }

        private static bool IsOption(string arg, string longName, string slashName)
        {
            return string.Equals(arg, longName, StringComparison.OrdinalIgnoreCase) ||
                   string.Equals(arg, slashName, StringComparison.OrdinalIgnoreCase);
        }

        // Quote one argument with the rules used by CommandLineToArgvW / the MSVC runtime
        // (which Chromium/Electron also use).
        private static string QuoteArg(string arg)
        {
            var sb = new StringBuilder("\"");
            int backslashes = 0;
            foreach (char c in arg)
            {
                if (c == '\\')
                {
                    backslashes++;
                    continue;
                }
                // Backslashes are only special when followed by a quote
                sb.Append('\\', c == '"' ? backslashes * 2 + 1 : backslashes);
                sb.Append(c);
                backslashes = 0;
            }
            // Trailing backslashes precede the closing quote: double them
            sb.Append('\\', backslashes * 2);
            sb.Append('"');
            return sb.ToString();
        }

        private static bool LaunchClaude()
        {
            // Attempt 1: start claude.exe directly (no cmd.exe in between, so no cmd quoting/metacharacter issues)
            try
            {
                var psi = new ProcessStartInfo(_best.ExePath, _launchArgs);
                psi.WorkingDirectory = _best.AppDir;
                psi.UseShellExecute = false;
                using (Process.Start(psi)) { }
                return true;
            }
            catch { }

            // Attempt 2: ShellExecute (same mechanism used by `cmd /c start`)
            try
            {
                var psi = new ProcessStartInfo(_best.ExePath, _launchArgs);
                psi.WorkingDirectory = _best.AppDir;
                psi.UseShellExecute = true;
                using (Process.Start(psi)) { }
                return true;
            }
            catch
            {
                return false;
            }
        }

        // ---------------------------------------------------------------------
        // Guardian: keeps the flag on after Claude auto-updates and restarts itself
        // ---------------------------------------------------------------------

        private static void RunGuardian()
        {
            bool createdNew;
            Mutex mutex;
            try
            {
                mutex = new Mutex(false, GuardianMutexName, out createdNew);
            }
            catch
            {
                return;
            }

            using (mutex)
            {
                if (!createdNew) return; // another Cloudy is already guarding Claude

                int idleMs = 0;
                int restarts = 0;
                while (true)
                {
                    List<ClaudeProc> procs = ScanClaudeProcesses();

                    if (AnyMainWithoutFlag(procs))
                    {
                        // Claude was (re)started without the flag: typically the auto-updater
                        // relaunching the new version. Relaunch it with the flag.
                        if (restarts >= GuardMaxRestarts)
                        {
                            MessageBox.Show(
                                "Claude Desktop continua a ripartire senza --disable-gpu-compositing.\n\n" +
                                "Cloudy ha smesso di riavviarlo automaticamente per evitare un ciclo.\n" +
                                "Chiudi Claude e riaprilo con Cloudy (oppure usa Cloudy.exe --restart).",
                                "Cloudy - Guardiano",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Warning
                            );
                            break;
                        }
                        restarts++;
                        KillDesktopClaude();
                        FindLatestClaude(); // the update moved Claude to a new versioned folder
                        if (_best == null || !LaunchClaude()) break;
                        idleMs = 0;
                        Thread.Sleep(GuardPollMs);
                        continue;
                    }

                    ClaudeProc main = procs.Find(p => p.IsMain);
                    if (main != null)
                    {
                        if (ProcessHasGpuFlag(main.Pid) == 1) restarts = 0;
                        idleMs = 0;
                        IntPtr h = Native.OpenProcess(Native.SYNCHRONIZE, false, (uint)main.Pid);
                        if (h != IntPtr.Zero)
                        {
                            // Zero CPU while Claude runs: just wait for the main process to exit
                            Native.WaitForSingleObject(h, Native.INFINITE);
                            Native.CloseHandle(h);
                            continue;
                        }
                    }
                    else if (idleMs >= GuardGraceMs)
                    {
                        // Claude was closed and nobody relaunched it: we're done
                        break;
                    }

                    Thread.Sleep(GuardPollMs);
                    idleMs += GuardPollMs;
                }
            }
        }

        private static void ShowStatus()
        {
            var sb = new StringBuilder();
            sb.Append("Claude Desktop rilevato correttamente!\n\n");
            sb.Append("Versione rilevata: ").Append(_best.ParsedVersion).Append("\n\n");
            sb.Append("Percorso eseguibile:\n").Append(_best.ExePath).Append("\n\n");
            sb.Append("Argomenti di avvio:\n").Append(_launchArgs).Append("\n\n");

            sb.Append("Istanza in esecuzione: ");
            int shown = 0;
            foreach (ClaudeProc p in ScanClaudeProcesses())
            {
                if (!p.IsMain) continue;
                if (shown++ > 0) sb.Append(", ");
                sb.Append("PID ").Append(p.Pid);
                int flag = ProcessHasGpuFlag(p.Pid);
                if (flag == 1) sb.Append(" (con --disable-gpu-compositing)");
                else if (flag == 0) sb.Append(" (SENZA --disable-gpu-compositing: avvia Cloudy per correggerla)");
                else sb.Append(" (flag non verificabile)");
            }
            if (shown == 0) sb.Append("nessuna");

            bool guardianActive = false;
            try
            {
                using (Mutex.OpenExisting(GuardianMutexName))
                {
                    guardianActive = true;
                }
            }
            catch { }
            sb.Append("\nGuardiano aggiornamenti: ").Append(guardianActive ? "attivo" : "non attivo");

            MessageBox.Show(sb.ToString(), "Cloudy - Stato Launcher", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        // ---------------------------------------------------------------------
        // Running Claude Desktop processes
        // ---------------------------------------------------------------------

        private static bool ContainsI(string s, string sub)
        {
            return s != null && s.IndexOf(sub, StringComparison.OrdinalIgnoreCase) >= 0;
        }

        // Only Claude *Desktop* binaries. The Claude Code CLI is also called claude.exe
        // (e.g. %USERPROFILE%\.local\bin\claude.exe) and must never be killed by us.
        private static bool IsDesktopClaudeImage(string path)
        {
            if (string.IsNullOrEmpty(path)) return false;
            if (ContainsI(path, @"\WindowsApps\") &&
                (ContainsI(path, @"\Claude_") || ContainsI(path, @"\Anthropic.Claude_") || ContainsI(path, "pzs8sxrjxfjjc")))
            {
                return true;
            }
            if (ContainsI(path, @"\AnthropicClaude\")) return true;
            if (ContainsI(path, @"\Programs\Claude\claude.exe")) return true;
            if (_best != null && !string.IsNullOrEmpty(_best.AppDir) &&
                path.StartsWith(_best.AppDir.TrimEnd('\\') + @"\", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
            return false;
        }

        private static string GetImagePath(IntPtr hProcess)
        {
            var sb = new StringBuilder(1024);
            uint size = (uint)sb.Capacity;
            if (!Native.QueryFullProcessImageNameW(hProcess, 0, sb, ref size)) return null;
            return sb.ToString();
        }

        private static List<ClaudeProc> ScanClaudeProcesses()
        {
            var list = new List<ClaudeProc>();
            IntPtr snap = Native.CreateToolhelp32Snapshot(Native.TH32CS_SNAPPROCESS, 0);
            if (snap == IntPtr.Zero || snap == Native.INVALID_HANDLE_VALUE) return list;

            try
            {
                var pe = new Native.PROCESSENTRY32W();
                pe.dwSize = (uint)Marshal.SizeOf(typeof(Native.PROCESSENTRY32W));
                bool ok = Native.Process32FirstW(snap, ref pe);
                while (ok)
                {
                    if (string.Equals(pe.szExeFile, "claude.exe", StringComparison.OrdinalIgnoreCase))
                    {
                        IntPtr h = Native.OpenProcess(Native.PROCESS_QUERY_LIMITED_INFORMATION | Native.SYNCHRONIZE, false, pe.th32ProcessID);
                        if (h != IntPtr.Zero)
                        {
                            try
                            {
                                // Skip processes that already exited (zombies still referenced by a handle)
                                if (Native.WaitForSingleObject(h, 0) == Native.WAIT_TIMEOUT && IsDesktopClaudeImage(GetImagePath(h)))
                                {
                                    var cp = new ClaudeProc();
                                    cp.Pid = (int)pe.th32ProcessID;
                                    cp.ParentPid = (int)pe.th32ParentProcessID;
                                    cp.IsMain = true;
                                    list.Add(cp);
                                }
                            }
                            finally
                            {
                                Native.CloseHandle(h);
                            }
                        }
                    }
                    ok = Native.Process32NextW(snap, ref pe);
                }
            }
            catch { }
            finally
            {
                Native.CloseHandle(snap);
            }

            // Renderer/GPU/utility/crashpad helpers are children of the main process
            foreach (ClaudeProc p in list)
            {
                foreach (ClaudeProc q in list)
                {
                    if (!ReferenceEquals(p, q) && p.ParentPid == q.Pid)
                    {
                        p.IsMain = false;
                        break;
                    }
                }
            }

            // A helper can outlive its main process (e.g. crashpad-handler): Chromium marks
            // every helper with --type=..., the main process never has it.
            foreach (ClaudeProc p in list)
            {
                if (!p.IsMain) continue;
                if (ContainsI(ReadCommandLine(p.Pid), "--type=")) p.IsMain = false;
            }
            return list;
        }

        // Returns the command line of a process, or null if it cannot be read (requires Windows 8.1+)
        private static string ReadCommandLine(int pid)
        {
            IntPtr h = Native.OpenProcess(Native.PROCESS_QUERY_LIMITED_INFORMATION, false, (uint)pid);
            if (h == IntPtr.Zero) return null;

            const int size = 65536 + 64; // UNICODE_STRING header + max 32767 chars
            IntPtr buf = IntPtr.Zero;
            try
            {
                buf = Marshal.AllocHGlobal(size);
                int returnLength;
                int status = Native.NtQueryInformationProcess(h, Native.ProcessCommandLineInformation, buf, size, out returnLength);
                if (status < 0) return null;

                // UNICODE_STRING { USHORT Length; USHORT MaximumLength; PWSTR Buffer; }
                int lengthBytes = (ushort)Marshal.ReadInt16(buf, 0);
                IntPtr str = Marshal.ReadIntPtr(buf, IntPtr.Size);
                if (str == IntPtr.Zero) return null;
                return Marshal.PtrToStringUni(str, lengthBytes / 2);
            }
            catch
            {
                return null;
            }
            finally
            {
                if (buf != IntPtr.Zero) Marshal.FreeHGlobal(buf);
                Native.CloseHandle(h);
            }
        }

        // 1 = command line contains --disable-gpu-compositing, 0 = it does not, -1 = unknown
        private static int ProcessHasGpuFlag(int pid)
        {
            string cmd = ReadCommandLine(pid);
            if (cmd == null) return -1;
            return ContainsI(cmd, GpuFlag) ? 1 : 0;
        }

        private static bool AnyMainWithoutFlag(List<ClaudeProc> procs)
        {
            foreach (ClaudeProc p in procs)
            {
                if (p.IsMain && ProcessHasGpuFlag(p.Pid) == 0) return true;
            }
            return false;
        }

        // Terminate every Claude Desktop process (main first, then helpers) and wait for them to exit,
        // so that Electron's single-instance lock is released before we relaunch.
        private static void KillDesktopClaude()
        {
            var handles = new List<IntPtr>();
            List<ClaudeProc> procs = ScanClaudeProcesses();

            for (int pass = 0; pass < 2; pass++)
            {
                foreach (ClaudeProc p in procs)
                {
                    if ((pass == 0) != p.IsMain) continue;
                    IntPtr h = Native.OpenProcess(
                        Native.PROCESS_TERMINATE | Native.SYNCHRONIZE | Native.PROCESS_QUERY_LIMITED_INFORMATION,
                        false,
                        (uint)p.Pid);
                    if (h == IntPtr.Zero) continue;
                    // Re-check the image in case the PID was recycled since the snapshot
                    if (!IsDesktopClaudeImage(GetImagePath(h)))
                    {
                        Native.CloseHandle(h);
                        continue;
                    }
                    Native.TerminateProcess(h, 1);
                    handles.Add(h);
                }
            }

            Stopwatch sw = Stopwatch.StartNew();
            foreach (IntPtr h in handles)
            {
                long remaining = KillTimeoutMs - sw.ElapsedMilliseconds;
                Native.WaitForSingleObject(h, remaining > 0 ? (uint)remaining : 0);
                Native.CloseHandle(h);
            }
            if (handles.Count > 0) Thread.Sleep(300);
        }

        // ---------------------------------------------------------------------
        // Locating the latest installed Claude Desktop
        // ---------------------------------------------------------------------

        private static void FindLatestClaude()
        {
            _best = null;

            string[] regPaths = new[]
            {
                @"Software\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppModel\Repository\Packages",
                @"Software\Classes\ActivatableClasses\Package",
                @"Software\Classes\Extensions\ContractId\Windows.Launch\PackageId"
            };

            foreach (string regPath in regPaths)
            {
                ScanRegistryKey(Registry.CurrentUser, regPath);
                ScanRegistryKey(Registry.LocalMachine, regPath);
            }

            string progW6432 = Environment.GetEnvironmentVariable("ProgramW6432");
            if (!string.IsNullOrEmpty(progW6432))
            {
                ScanDirectoryPattern(Path.Combine(progW6432, "WindowsApps"), "Claude_*");
            }
            ScanDirectoryPattern(@"C:\Program Files\WindowsApps", "Claude_*");

            string localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            if (!string.IsNullOrEmpty(localAppData))
            {
                string anthropicDir = Path.Combine(localAppData, "AnthropicClaude");
                ScanDirectoryPattern(anthropicDir, "app-*");
                ConsiderPackageDir(anthropicDir, "");
                ConsiderPackageDir(Path.Combine(localAppData, @"Programs\claude"), "");
                ConsiderPackageDir(Path.Combine(localAppData, @"Programs\Claude"), "");
            }
        }

        private static void ScanRegistryKey(RegistryKey root, string subKeyPath)
        {
            try
            {
                using (RegistryKey key = root.OpenSubKey(subKeyPath, false))
                {
                    if (key == null) return;
                    foreach (string subName in key.GetSubKeyNames())
                    {
                        if (subName.StartsWith("Claude_", StringComparison.OrdinalIgnoreCase) ||
                            subName.StartsWith("Anthropic.Claude_", StringComparison.OrdinalIgnoreCase) ||
                            subName.IndexOf("pzs8sxrjxfjjc", StringComparison.OrdinalIgnoreCase) >= 0)
                        {
                            try
                            {
                                using (RegistryKey sub = key.OpenSubKey(subName, false))
                                {
                                    if (sub != null)
                                    {
                                        string rootFolder = sub.GetValue("PackageRootFolder") as string;
                                        if (!string.IsNullOrEmpty(rootFolder))
                                        {
                                            ConsiderPackageDir(rootFolder, subName);
                                        }
                                    }
                                }
                            }
                            catch { }

                            ConsiderPackageDir(Path.Combine(@"C:\Program Files\WindowsApps", subName), subName);
                        }
                    }
                }
            }
            catch { }
        }

        private static void ScanDirectoryPattern(string baseDir, string pattern)
        {
            try
            {
                if (!Directory.Exists(baseDir)) return;
                foreach (string dir in Directory.GetDirectories(baseDir, pattern))
                {
                    ConsiderPackageDir(dir, Path.GetFileName(dir));
                }
            }
            catch { }
        }

        private static void ConsiderPackageDir(string pkgDir, string versionHint)
        {
            if (string.IsNullOrEmpty(pkgDir)) return;

            try
            {
                string candidateExe = Path.Combine(pkgDir, @"app\claude.exe");
                string candidateAppDir = Path.Combine(pkgDir, "app");

                if (!File.Exists(candidateExe))
                {
                    candidateExe = Path.Combine(pkgDir, "claude.exe");
                    candidateAppDir = pkgDir;
                    if (!File.Exists(candidateExe)) return;
                }

                Version ver = ParseVersion(!string.IsNullOrEmpty(versionHint) ? versionHint : pkgDir);
                if (_best == null || ver.CompareTo(_best.ParsedVersion) > 0)
                {
                    _best = new Candidate
                    {
                        ExePath = candidateExe,
                        AppDir = candidateAppDir,
                        ParsedVersion = ver
                    };
                }
            }
            catch { }
        }

        private static Version ParseVersion(string text)
        {
            if (!string.IsNullOrEmpty(text))
            {
                Match m = Regex.Match(text, @"(\d+)\.(\d+)(?:\.(\d+))?(?:\.(\d+))?");
                int major, minor, build = 0, rev = 0;
                if (m.Success &&
                    int.TryParse(m.Groups[1].Value, out major) &&
                    int.TryParse(m.Groups[2].Value, out minor) &&
                    (!m.Groups[3].Success || int.TryParse(m.Groups[3].Value, out build)) &&
                    (!m.Groups[4].Success || int.TryParse(m.Groups[4].Value, out rev)))
                {
                    return new Version(major, minor, build, rev);
                }
            }
            return new Version(0, 0, 0, 1);
        }

        private static class Native
        {
            public const uint PROCESS_TERMINATE = 0x0001;
            public const uint PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;
            public const uint SYNCHRONIZE = 0x00100000;
            public const uint TH32CS_SNAPPROCESS = 0x00000002;
            public const uint WAIT_TIMEOUT = 0x00000102;
            public const uint INFINITE = 0xFFFFFFFF;
            public const int ProcessCommandLineInformation = 60;
            public static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);

            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
            public struct PROCESSENTRY32W
            {
                public uint dwSize;
                public uint cntUsage;
                public uint th32ProcessID;
                public IntPtr th32DefaultHeapID;
                public uint th32ModuleID;
                public uint cntThreads;
                public uint th32ParentProcessID;
                public int pcPriClassBase;
                public uint dwFlags;
                [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
                public string szExeFile;
            }

            [DllImport("kernel32.dll", SetLastError = true)]
            public static extern IntPtr CreateToolhelp32Snapshot(uint dwFlags, uint th32ProcessID);

            [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true)]
            public static extern bool Process32FirstW(IntPtr hSnapshot, ref PROCESSENTRY32W lppe);

            [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true)]
            public static extern bool Process32NextW(IntPtr hSnapshot, ref PROCESSENTRY32W lppe);

            [DllImport("kernel32.dll", SetLastError = true)]
            public static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, uint dwProcessId);

            [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true)]
            public static extern bool QueryFullProcessImageNameW(IntPtr hProcess, uint dwFlags, StringBuilder lpExeName, ref uint lpdwSize);

            [DllImport("kernel32.dll", SetLastError = true)]
            public static extern bool TerminateProcess(IntPtr hProcess, uint uExitCode);

            [DllImport("kernel32.dll", SetLastError = true)]
            public static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

            [DllImport("kernel32.dll", SetLastError = true)]
            public static extern bool CloseHandle(IntPtr hObject);

            [DllImport("ntdll.dll")]
            public static extern int NtQueryInformationProcess(IntPtr hProcess, int processInformationClass, IntPtr processInformation, int processInformationLength, out int returnLength);
        }
    }
}
