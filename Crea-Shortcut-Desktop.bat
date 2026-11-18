@echo off
setlocal
cd /d "%~dp0"

if not exist "%~dp0Cloudy.exe" (
    echo [!] Cloudy.exe non trovato nella cartella corrente, avvio compilazione rapida...
    call "%~dp0Build-Local.bat"
)

rem La cartella passa a PowerShell tramite variabile d'ambiente, cosi' i percorsi
rem con apostrofi (es. C:\Users\D'Angelo) non rompono il comando.
rem L'icona e' quella di Cloudy.exe: claude.exe sta in una cartella WindowsApps
rem versionata che viene cancellata a ogni aggiornamento, e il collegamento
rem resterebbe con l'icona vuota.
set "CLOUDY_DIR=%~dp0"

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$dir = $env:CLOUDY_DIR;" ^
  "$exe = Join-Path $dir 'Cloudy.exe';" ^
  "$desktop = [Environment]::GetFolderPath('Desktop');" ^
  "$lnkPath = Join-Path $desktop 'Claude.lnk';" ^
  "$ws = New-Object -ComObject WScript.Shell;" ^
  "$s = $ws.CreateShortcut($lnkPath);" ^
  "$s.TargetPath = $exe;" ^
  "$s.WorkingDirectory = $dir;" ^
  "$s.Description = 'Avvia Claude Desktop con --disable-gpu-compositing (G-Sync Flicker Fix)';" ^
  "$s.IconLocation = $exe + ',0';" ^
  "$s.Save();" ^
  "Write-Host '[OK] Collegamento creato sul Desktop:' $lnkPath -ForegroundColor Green"

pause
