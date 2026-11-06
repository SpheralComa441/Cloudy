using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Microsoft.Win32;

namespace Cloudy
{
    internal static class Program
    {
        private sealed class Candidate
        {
            public string ExePath = "";
            public string AppDir = "";
            public Version ParsedVersion = new Version(0, 0, 0, 0);
        }

        private static Candidate _best;

        [STAThread]
        private static int Main(string[] args)
        {
            bool showStatus = false;
            bool restartClaude = false;
            var extraArgs = new List<string>();

            foreach (string arg in args)
            {
                if (string.Equals(arg, "--status", StringComparison.OrdinalIgnoreCase) ||
                    string.Equals(arg, "/status", StringComparison.OrdinalIgnoreCase))
                {
                    showStatus = true;
                }
                else if (string.Equals(arg, "--restart", StringComparison.OrdinalIgnoreCase) ||
                         string.Equals(arg, "/restart", StringComparison.OrdinalIgnoreCase))
                {
                    restartClaude = true;
                }
                else if (!string.Equals(arg, "--disable-gpu-compositing", StringComparison.OrdinalIgnoreCase))
                {
                    extraArgs.Add("\"" + arg.Replace("\"", "\\\"") + "\"");
                }
            }

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

            string launchArgs = "--disable-gpu-compositing";
            if (extraArgs.Count > 0)
            {
                launchArgs += " " + string.Join(" ", extraArgs.ToArray());
            }

            if (showStatus)
            {
                MessageBox.Show(
                    "Claude Desktop rilevato correttamente!\n\n" +
                    "Versione rilevata: " + _best.ParsedVersion + "\n\n" +
                    "Percorso eseguibile:\n" + _best.ExePath + "\n\n" +
                    "Argomenti di avvio:\n" + launchArgs,
                    "Cloudy - Stato Launcher",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information
                );
                return 0;
            }

            if (restartClaude)
            {
                try
                {
                    foreach (Process p in Process.GetProcessesByName("claude"))
                    {
                        try { p.Kill(); } catch { }
                    }
                    System.Threading.Thread.Sleep(350);
                }
                catch { }
            }

            try
            {
                var psi = new ProcessStartInfo
                {
                    FileName = "cmd.exe",
                    Arguments = "/c start \"\" \"" + _best.ExePath + "\" " + launchArgs,
                    WorkingDirectory = _best.AppDir,
                    CreateNoWindow = true,
                    UseShellExecute = false,
                    WindowStyle = ProcessWindowStyle.Hidden
                };
                Process.Start(psi);
                return 0;
            }
            catch
            {
                try
                {
                    var fallbackPsi = new ProcessStartInfo
                    {
                        FileName = _best.ExePath,
                        Arguments = launchArgs,
                        WorkingDirectory = _best.AppDir,
                        UseShellExecute = true
                    };
                    Process.Start(fallbackPsi);
                    return 0;
                }
                catch (Exception ex)
                {
                    MessageBox.Show(
                        "Impossibile avviare Claude Desktop:\n" + ex.Message,
                        "Cloudy - Errore di avvio",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error
                    );
                    return 1;
                }
            }
        }

        private static void FindLatestClaude()
        {
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

        private static Version ParseVersion(string text)
        {
            if (!string.IsNullOrEmpty(text))
            {
                Match m = Regex.Match(text, @"(\d+)\.(\d+)(?:\.(\d+))?(?:\.(\d+))?");
                if (m.Success)
                {
                    int major = int.Parse(m.Groups[1].Value);
                    int minor = int.Parse(m.Groups[2].Value);
                    int build = m.Groups[3].Success ? int.Parse(m.Groups[3].Value) : 0;
                    int rev = m.Groups[4].Success ? int.Parse(m.Groups[4].Value) : 0;
                    return new Version(major, minor, build, rev);
                }
            }
            return new Version(0, 0, 0, 1);
        }
    }
}
