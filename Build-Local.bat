@echo off
setlocal
cd /d "%~dp0"

set "CSC=%SystemRoot%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%CSC%" set "CSC=%SystemRoot%\Microsoft.NET\Framework\v4.0.30319\csc.exe"

if not exist "%CSC%" (
    echo [ERRORE] Compilatore .NET Framework csc.exe non trovato.
    pause
    exit /b 1
)

echo Compilazione di Cloudy.exe in corso...
"%CSC%" /nologo /target:winexe /optimize+ /out:"%~dp0Cloudy.exe" /win32icon:"%~dp0assets\cloudy.ico" "%~dp0src\Cloudy.cs"

if %ERRORLEVEL% EQU 0 (
    echo [OK] Cloudy.exe compilato con successo!
) else (
    echo [ERRORE] Compilazione fallita.
    pause
    exit /b 1
)
