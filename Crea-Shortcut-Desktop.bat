@echo off
setlocal
cd /d "%~dp0"

if not exist "%~dp0Cloudy.exe" (
    echo [!] Cloudy.exe non trovato nella cartella corrente, avvio compilazione rapida...
    call "%~dp0Build-Local.bat"
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$exe = Join-Path '%~dp0' 'Cloudy.exe';" ^
  "$desktop = [Environment]::GetFolderPath('Desktop');" ^
  "$lnkPath = Join-Path $desktop 'Claude.lnk';" ^
  "$pkg = Get-AppxPackage -Name '*Claude*' | Sort-Object Version -Descending | Select-Object -First 1;" ^
  "$icon = $exe;" ^
  "if ($pkg -and (Test-Path (Join-Path $pkg.InstallLocation 'app\claude.exe'))) { $icon = (Join-Path $pkg.InstallLocation 'app\claude.exe') + ',0'; }" ^
  "$ws = New-Object -ComObject WScript.Shell;" ^
  "$s = $ws.CreateShortcut($lnkPath);" ^
  "$s.TargetPath = $exe;" ^
  "$s.WorkingDirectory = '%~dp0';" ^
  "$s.Description = 'Avvia Claude Desktop con --disable-gpu-compositing (G-Sync Flicker Fix)';" ^
  "$s.IconLocation = $icon;" ^
  "$s.Save();" ^
  "Write-Host '[OK] Collegamento creato sul Desktop:' $lnkPath -ForegroundColor Green"

pause
