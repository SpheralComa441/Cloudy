#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
mkdir -p build

cat > build/kernel32.def << 'EOF'
LIBRARY kernel32.dll
EXPORTS
    GetCommandLineW
    LocalFree
    GetEnvironmentVariableW
    GetFileAttributesW
    FindFirstFileW
    FindNextFileW
    FindClose
    CreateProcessW
    WaitForSingleObject
    CloseHandle
    Sleep
    ExitProcess
    CreatePipe
    SetHandleInformation
    ReadFile
EOF

cat > build/advapi32.def << 'EOF'
LIBRARY advapi32.dll
EXPORTS
    RegOpenKeyExW
    RegEnumKeyExW
    RegQueryValueExW
    RegCloseKey
EOF

cat > build/user32.def << 'EOF'
LIBRARY user32.dll
EXPORTS
    MessageBoxW
EOF

cat > build/shell32.def << 'EOF'
LIBRARY shell32.dll
EXPORTS
    ShellExecuteW
    CommandLineToArgvW
EOF

for dll in kernel32 advapi32 user32 shell32; do
    llvm-dlltool -m i386:x86-64 -d "build/${dll}.def" -l "build/${dll}.lib"
done

llvm-rc /FO build/cloudy.res src/cloudy.rc

clang --target=x86_64-pc-windows-msvc \
    -Os -fno-builtin -nostdlib -mno-stack-arg-probe \
    -Wall -Wextra \
    -c src/cloudy.c -o build/cloudy.obj

lld-link \
    /SUBSYSTEM:WINDOWS \
    /ENTRY:WinMainCRTStartup \
    /OUT:Cloudy.exe \
    build/cloudy.obj \
    build/cloudy.res \
    build/kernel32.lib \
    build/advapi32.lib \
    build/user32.lib \
    build/shell32.lib

rm -rf build
echo "Built Cloudy.exe ($(wc -c < Cloudy.exe) bytes)"
