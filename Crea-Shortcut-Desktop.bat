@echo off
setlocal
cd /d "%~dp0"

if not exist "%~dp0Cloudy.exe" (
    echo [ERRORE] Cloudy.exe non trovato in questa cartella.
    echo Scaricalo dalla pagina delle release e mettilo accanto a questo script:
    echo https://github.com/SpheralComa441/Cloudy/releases/latest
    pause
    exit /b 1
)

rem La cartella passa a PowerShell tramite variabile d'ambiente, cosi' i percorsi
rem con apostrofi (es. C:\Users\D'Angelo) non rompono il comando.
rem L'icona e' quella di Cloudy.exe: claude.exe sta in una cartella WindowsApps
rem versionata che viene cancellata a ogni aggiornamento, e il collegamento
rem resterebbe con l'icona vuota.
rem La modalita' (gpu / dcomp / both) si sceglie con Cloudy.ini accanto a Cloudy.exe.
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
  "$s.Description = 'Avvia Claude Desktop con il workaround anti-flickering di Cloudy';" ^
  "$s.IconLocation = $exe + ',0';" ^
  "$s.Save();" ^
  "Write-Host '[OK] Collegamento creato sul Desktop:' $lnkPath -ForegroundColor Green"

pause
