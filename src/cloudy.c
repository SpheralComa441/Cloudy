/*
 * Cloudy - Ultra-lightweight launcher for Claude Desktop on Windows (G-Sync flicker fix)
 *
 * Automatically finds the latest installed version of Claude Desktop
 * (both MSIX/WindowsApps packages like Claude_1.10628.2.0_x64__pzs8sxrjxfjjc
 * and standard AppData installations) without requiring Administrator privileges,
 * and launches app\claude.exe with --disable-gpu-compositing.
 *
 * Zero dependencies, zero background processes, zero console window.
 */

typedef unsigned short WCHAR;
typedef unsigned int DWORD;
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned long long ULONG_PTR;
typedef long long LONG_PTR;
typedef void *HANDLE;
typedef void *HKEY;
typedef void *HINSTANCE;
typedef void *HWND;
typedef const WCHAR *LPCWSTR;
typedef WCHAR *LPWSTR;

#define NULL ((void *)0)
#define TRUE 1
#define FALSE 0
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010

#define HKEY_CURRENT_USER  ((HKEY)(ULONG_PTR)0x80000001ULL)
#define HKEY_LOCAL_MACHINE ((HKEY)(ULONG_PTR)0x80000002ULL)
#define KEY_READ           0x20019
#define ERROR_SUCCESS      0L

#define CREATE_NO_WINDOW   0x08000000
#define STARTF_USESHOWWINDOW 0x00000001
#define STARTF_USESTDHANDLES 0x00000100
#define SW_HIDE            0
#define SW_SHOWNORMAL      1
#define HANDLE_FLAG_INHERIT 0x00000001

#define MB_OK              0x00000000
#define MB_ICONERROR       0x00000010
#define MB_ICONINFORMATION 0x00000040

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

typedef struct _WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    WCHAR cFileName[260];
    WCHAR cAlternateFileName[14];
} WIN32_FIND_DATAW;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    void *lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES;

typedef struct _STARTUPINFOW {
    DWORD cb;
    LPWSTR lpReserved;
    LPWSTR lpDesktop;
    LPWSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    unsigned short wShowWindow;
    unsigned short cbReserved2;
    BYTE *lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOW;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION;

/* Win32 API imports */
__declspec(dllimport) LPWSTR __stdcall GetCommandLineW(void);
__declspec(dllimport) LPWSTR * __stdcall CommandLineToArgvW(LPCWSTR lpCmdLine, int *pNumArgs);
__declspec(dllimport) HANDLE __stdcall LocalFree(HANDLE hMem);
__declspec(dllimport) DWORD __stdcall GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);
__declspec(dllimport) DWORD __stdcall GetFileAttributesW(LPCWSTR lpFileName);
__declspec(dllimport) HANDLE __stdcall FindFirstFileW(LPCWSTR lpFileName, WIN32_FIND_DATAW *lpFindFileData);
__declspec(dllimport) BOOL __stdcall FindNextFileW(HANDLE hFindFile, WIN32_FIND_DATAW *lpFindFileData);
__declspec(dllimport) BOOL __stdcall FindClose(HANDLE hFindFile);
__declspec(dllimport) BOOL __stdcall CreateProcessW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    SECURITY_ATTRIBUTES *lpProcessAttributes,
    SECURITY_ATTRIBUTES *lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    void *lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    STARTUPINFOW *lpStartupInfo,
    PROCESS_INFORMATION *lpProcessInformation
);
__declspec(dllimport) DWORD __stdcall WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE hObject);
__declspec(dllimport) void __stdcall Sleep(DWORD dwMilliseconds);
__declspec(dllimport) void __stdcall ExitProcess( unsigned int uExitCode);
__declspec(dllimport) BOOL __stdcall CreatePipe(HANDLE *hReadPipe, HANDLE *hWritePipe, SECURITY_ATTRIBUTES *lpPipeAttributes, DWORD nSize);
__declspec(dllimport) BOOL __stdcall SetHandleInformation(HANDLE hObject, DWORD dwMask, DWORD dwFlags);
__declspec(dllimport) BOOL __stdcall ReadFile(HANDLE hFile, void *lpBuffer, DWORD nNumberOfBytesToRead, DWORD *lpNumberOfBytesRead, void *lpOverlapped);

__declspec(dllimport) long __stdcall RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, DWORD samDesired, HKEY *phkResult);
__declspec(dllimport) long __stdcall RegEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD *lpcchName, DWORD *lpReserved, LPWSTR lpClass, DWORD *lpcchClass, FILETIME *lpftLastWriteTime);
__declspec(dllimport) long __stdcall RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD *lpReserved, DWORD *lpType, BYTE *lpData, DWORD *lpcbData);
__declspec(dllimport) long __stdcall RegCloseKey(HKEY hKey);

__declspec(dllimport) int __stdcall MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, unsigned int uType);
__declspec(dllimport) HINSTANCE __stdcall ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, int nShowCmd);

/* Freestanding memory helpers */
void *memset(void *dest, int c, unsigned long long count) {
    unsigned char *p = (unsigned char *)dest;
    while (count--) *p++ = (unsigned char)c;
    return dest;
}

void *memcpy(void *dest, const void *src, unsigned long long count) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (count--) *d++ = *s++;
    return dest;
}

/* Wide string helpers */
static int wlen(LPCWSTR s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static void wcopy(LPWSTR dst, int cap, LPCWSTR src) {
    int i = 0;
    if (cap <= 0) return;
    if (src) {
        while (src[i] && i < cap - 1) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = 0;
}

static void wcat(LPWSTR dst, int cap, LPCWSTR src) {
    int dlen = wlen(dst);
    int i = 0;
    if (!src || dlen >= cap - 1) return;
    while (src[i] && (dlen + i) < cap - 1) {
        dst[dlen + i] = src[i];
        i++;
    }
    dst[dlen + i] = 0;
}

static WCHAR wlower(WCHAR c) {
    if (c >= L'A' && c <= L'Z') return (WCHAR)(c + (L'a' - L'A'));
    return c;
}

static int weq_i(LPCWSTR a, LPCWSTR b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && b[i]) {
        if (wlower(a[i]) != wlower(b[i])) return 0;
        i++;
    }
    return a[i] == 0 && b[i] == 0;
}

static int wstarts_with_i(LPCWSTR str, LPCWSTR prefix) {
    int i = 0;
    if (!str || !prefix) return 0;
    while (prefix[i]) {
        if (wlower(str[i]) != wlower(prefix[i])) return 0;
        i++;
    }
    return 1;
}

static int wcontains_i(LPCWSTR hay, LPCWSTR needle) {
    int i, j;
    if (!hay || !needle || !needle[0]) return 0;
    for (i = 0; hay[i]; i++) {
        for (j = 0; needle[j]; j++) {
            if (!hay[i + j] || wlower(hay[i + j]) != wlower(needle[j])) break;
        }
        if (!needle[j]) return 1;
    }
    return 0;
}

static int file_exists(LPCWSTR path) {
    DWORD attr;
    if (!path || !path[0]) return 0;
    attr = GetFileAttributesW(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

/* Extract up to 4 numeric version components from a package/folder name like:
   Claude_1.10628.2.0_x64__pzs8sxrjxfjjc or app-1.10628.2 */
static void parse_version(LPCWSTR text, unsigned int ver[4]) {
    int i = 0;
    ver[0] = ver[1] = ver[2] = ver[3] = 0;
    if (!text) return;

    while (text[i]) {
        if (text[i] >= L'0' && text[i] <= L'9') {
            /* Check if this digit sequence contains at least one dot followed by a digit */
            int j = i;
            int dots = 0;
            while ((text[j] >= L'0' && text[j] <= L'9') || text[j] == L'.') {
                if (text[j] == L'.') dots++;
                j++;
            }
            if (dots >= 1) {
                int part = 0;
                int k = i;
                while (k < j && part < 4) {
                    unsigned int val = 0;
                    while (k < j && text[k] >= L'0' && text[k] <= L'9') {
                        val = val * 10 + (unsigned int)(text[k] - L'0');
                        k++;
                    }
                    ver[part++] = val;
                    if (k < j && text[k] == L'.') k++;
                }
                return;
            }
            i = j;
        } else {
            i++;
        }
    }
    /* Fallback non-zero lowest priority if no version numbers in path */
    ver[3] = 1;
}

static int compare_version(const unsigned int a[4], const unsigned int b[4]) {
    int i;
    for (i = 0; i < 4; i++) {
        if (a[i] > b[i]) return 1;
        if (a[i] < b[i]) return -1;
    }
    return 0;
}

typedef struct {
    WCHAR exe_path[1024];
    WCHAR app_dir[1024];
    WCHAR version_str[128];
    unsigned int ver[4];
    int found;
} Candidate;

static Candidate g_best;

static void uint_to_wstr(unsigned int v, LPWSTR out, int cap) {
    WCHAR tmp[16];
    int n = 0, i = 0;
    if (v == 0) {
        wcopy(out, cap, L"0");
        return;
    }
    while (v > 0 && n < 15) {
        tmp[n++] = (WCHAR)(L'0' + (v % 10));
        v /= 10;
    }
    while (n > 0 && i < cap - 1) {
        out[i++] = tmp[--n];
    }
    out[i] = 0;
}

static void format_version(const unsigned int ver[4], LPWSTR out, int cap) {
    WCHAR num[16];
    int i;
    out[0] = 0;
    for (i = 0; i < 4; i++) {
        if (i > 0) wcat(out, cap, L".");
        uint_to_wstr(ver[i], num, 16);
        wcat(out, cap, num);
    }
}

static void consider_package_dir(LPCWSTR pkg_dir, LPCWSTR ver_hint) {
    static WCHAR candidate_exe[1024];
    static WCHAR candidate_dir[1024];
    unsigned int ver[4];
    int len;

    if (!pkg_dir || !pkg_dir[0]) return;

    /* Normalize trailing slash */
    wcopy(candidate_dir, 1024, pkg_dir);
    len = wlen(candidate_dir);
    while (len > 0 && (candidate_dir[len - 1] == L'\\' || candidate_dir[len - 1] == L'/')) {
        candidate_dir[--len] = 0;
    }

    /* Check <pkg_dir>\app\claude.exe first (MSIX layout) */
    wcopy(candidate_exe, 1024, candidate_dir);
    wcat(candidate_exe, 1024, L"\\app\\claude.exe");
    if (file_exists(candidate_exe)) {
        wcat(candidate_dir, 1024, L"\\app");
    } else {
        /* Check <pkg_dir>\claude.exe (AppData / unpacked layout) */
        wcopy(candidate_exe, 1024, candidate_dir);
        wcat(candidate_exe, 1024, L"\\claude.exe");
        if (!file_exists(candidate_exe)) {
            return;
        }
    }

    parse_version(ver_hint && ver_hint[0] ? ver_hint : pkg_dir, ver);
    if (!g_best.found || compare_version(ver, g_best.ver) > 0) {
        int i;
        wcopy(g_best.exe_path, 1024, candidate_exe);
        wcopy(g_best.app_dir, 1024, candidate_dir);
        for (i = 0; i < 4; i++) g_best.ver[i] = ver[i];
        format_version(ver, g_best.version_str, 128);
        g_best.found = 1;
    }
}

static int is_claude_package_name(LPCWSTR name) {
    if (!name || !name[0]) return 0;
    if (wstarts_with_i(name, L"Claude_") || wstarts_with_i(name, L"Anthropic.Claude_") || wcontains_i(name, L"pzs8sxrjxfjjc")) {
        return 1;
    }
    return 0;
}

static void check_windowsapps_for_pkgname(LPCWSTR pkg_full_name) {
    static WCHAR path_buf[1024];
    static WCHAR prog_files[512];

    if (GetEnvironmentVariableW(L"ProgramW6432", prog_files, 512) > 0) {
        wcopy(path_buf, 1024, prog_files);
        wcat(path_buf, 1024, L"\\WindowsApps\\");
        wcat(path_buf, 1024, pkg_full_name);
        consider_package_dir(path_buf, pkg_full_name);
    }

    wcopy(path_buf, 1024, L"C:\\Program Files\\WindowsApps\\");
    wcat(path_buf, 1024, pkg_full_name);
    consider_package_dir(path_buf, pkg_full_name);
}

/* Strategy 1: Enumerate AppModel Repository Packages in Registry (0ms, no Admin required) */
static void scan_registry_packages(HKEY root_key, LPCWSTR reg_path) {
    HKEY hKey = NULL;
    DWORD idx = 0;
    static WCHAR subkey_name[512];
    static WCHAR root_folder[1024];

    if (RegOpenKeyExW(root_key, reg_path, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return;
    }

    for (idx = 0; ; idx++) {
        DWORD name_len = 512;
        FILETIME ft;
        long res = RegEnumKeyExW(hKey, idx, subkey_name, &name_len, NULL, NULL, NULL, &ft);
        if (res != ERROR_SUCCESS) break;

        if (is_claude_package_name(subkey_name)) {
            HKEY hSub = NULL;
            if (RegOpenKeyExW(hKey, subkey_name, 0, KEY_READ, &hSub) == ERROR_SUCCESS) {
                DWORD type = 0;
                DWORD data_bytes = sizeof(root_folder) - sizeof(WCHAR);
                if (RegQueryValueExW(hSub, L"PackageRootFolder", NULL, &type, (BYTE *)root_folder, &data_bytes) == ERROR_SUCCESS) {
                    root_folder[data_bytes / sizeof(WCHAR)] = 0;
                    consider_package_dir(root_folder, subkey_name);
                }
                RegCloseKey(hSub);
            }
            check_windowsapps_for_pkgname(subkey_name);
        }
    }

    RegCloseKey(hKey);
}

/* Strategy 2: Scan directory pattern via FindFirstFileW */
static void scan_directory_pattern(LPCWSTR base_dir, LPCWSTR pattern) {
    static WCHAR search_path[1024];
    static WCHAR full_dir[1024];
    WIN32_FIND_DATAW fd;
    HANDLE hFind;

    if (!base_dir || !base_dir[0]) return;
    wcopy(search_path, 1024, base_dir);
    wcat(search_path, 1024, L"\\");
    wcat(search_path, 1024, pattern);

    hFind = FindFirstFileW(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            fd.cFileName[0] != L'.') {
            wcopy(full_dir, 1024, base_dir);
            wcat(full_dir, 1024, L"\\");
            wcat(full_dir, 1024, fd.cFileName);
            consider_package_dir(full_dir, fd.cFileName);
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
}

/* Strategy 3: Hidden PowerShell Get-AppxPackage fallback (only if Registry + Direct scan didn't find it) */
static void scan_via_powershell_fallback(void) {
    SECURITY_ATTRIBUTES sa;
    HANDLE hRead = NULL, hWrite = NULL;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    static WCHAR ps_cmd[512];
    static char out_buf[1024];
    static WCHAR wout_buf[1024];
    DWORD total_read = 0, bytes_read = 0;
    int i, widx = 0;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    memset(&pi, 0, sizeof(pi));
    wcopy(ps_cmd, 512, L"powershell.exe -NoProfile -NonInteractive -WindowStyle Hidden -Command \"(Get-AppxPackage -Name '*Claude*' | Sort-Object Version -Descending | Select-Object -First 1).InstallLocation\"");

    if (CreateProcessW(NULL, ps_cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hWrite);
        hWrite = NULL;

        while (total_read < sizeof(out_buf) - 1 &&
               ReadFile(hRead, out_buf + total_read, (DWORD)(sizeof(out_buf) - 1 - total_read), &bytes_read, NULL) &&
               bytes_read > 0) {
            total_read += bytes_read;
        }
        out_buf[total_read] = 0;

        WaitForSingleObject(pi.hProcess, 3000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        for (i = 0; i < (int)total_read && widx < 1020; i++) {
            char c = out_buf[i];
            if (c == '\r' || c == '\n') break;
            if ((unsigned char)c >= 32) {
                wout_buf[widx++] = (WCHAR)(unsigned char)c;
            }
        }
        wout_buf[widx] = 0;
        if (widx > 0) {
            consider_package_dir(wout_buf, wout_buf);
        }
    }

    if (hWrite) CloseHandle(hWrite);
    if (hRead) CloseHandle(hRead);
}

static void find_latest_claude(void) {
    static WCHAR env_buf[512];
    static WCHAR dir_buf[1024];

    memset(&g_best, 0, sizeof(g_best));

    /* 1. Registry AppModel Repository (instantaneous, works for standard users without Admin) */
    scan_registry_packages(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages"
    );
    scan_registry_packages(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages"
    );
    scan_registry_packages(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\ActivatableClasses\\Package"
    );
    scan_registry_packages(
        HKEY_CURRENT_USER,
        L"Software\\Classes\\Extensions\\ContractId\\Windows.Launch\\PackageId"
    );

    /* 2. Direct WindowsApps scan (if user has list permission on WindowsApps) */
    if (GetEnvironmentVariableW(L"ProgramW6432", env_buf, 512) > 0) {
        wcopy(dir_buf, 1024, env_buf);
        wcat(dir_buf, 1024, L"\\WindowsApps");
        scan_directory_pattern(dir_buf, L"Claude_*");
    }
    scan_directory_pattern(L"C:\\Program Files\\WindowsApps", L"Claude_*");

    /* 3. Standard %LOCALAPPDATA% installations (Squirrel / Exe installer) */
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", env_buf, 512) > 0) {
        wcopy(dir_buf, 1024, env_buf);
        wcat(dir_buf, 1024, L"\\AnthropicClaude");
        scan_directory_pattern(dir_buf, L"app-*");
        consider_package_dir(dir_buf, L"");

        wcopy(dir_buf, 1024, env_buf);
        wcat(dir_buf, 1024, L"\\Programs\\claude");
        consider_package_dir(dir_buf, L"");

        wcopy(dir_buf, 1024, env_buf);
        wcat(dir_buf, 1024, L"\\Programs\\Claude");
        consider_package_dir(dir_buf, L"");
    }

    /* 4. PowerShell Get-AppxPackage fallback if still not found */
    if (!g_best.found) {
        scan_via_powershell_fallback();
    }
}

static void kill_existing_claude(void) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    static WCHAR kill_cmd[128];

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    memset(&pi, 0, sizeof(pi));

    wcopy(kill_cmd, 128, L"taskkill.exe /F /IM claude.exe");
    if (CreateProcessW(NULL, kill_cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 3000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        Sleep(350);
    }
}

void __stdcall WinMainCRTStartup(void) {
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    int opt_status = 0;
    int opt_restart = 0;
    static WCHAR extra_args[1024];
    static WCHAR cmd_line[2048];
    static WCHAR shell_args[1200];
    static WCHAR msg_buf[2048];
    int i;

    extra_args[0] = 0;
    if (argv) {
        for (i = 1; i < argc; i++) {
            if (weq_i(argv[i], L"--status") || weq_i(argv[i], L"/status")) {
                opt_status = 1;
            } else if (weq_i(argv[i], L"--restart") || weq_i(argv[i], L"/restart")) {
                opt_restart = 1;
            } else if (!weq_i(argv[i], L"--disable-gpu-compositing")) {
                wcat(extra_args, 1024, L" \"");
                wcat(extra_args, 1024, argv[i]);
                wcat(extra_args, 1024, L"\"");
            }
        }
        LocalFree((HANDLE)argv);
    }

    find_latest_claude();

    if (!g_best.found) {
        MessageBoxW(
            NULL,
            L"Impossibile trovare l'installazione di Claude Desktop.\n\n"
            L"Percorsi controllati:\n"
            L"• C:\\Program Files\\WindowsApps\\Claude_*\\app\\claude.exe\n"
            L"• %LOCALAPPDATA%\\AnthropicClaude\\claude.exe\n\n"
            L"Assicurati che l'app Claude Desktop sia installata.",
            L"Cloudy - Claude non trovato",
            MB_OK | MB_ICONERROR
        );
        ExitProcess(1);
    }

    if (opt_status) {
        wcopy(msg_buf, 2048, L"Claude Desktop rilevato correttamente!\n\nVersione rilevata: ");
        wcat(msg_buf, 2048, g_best.version_str);
        wcat(msg_buf, 2048, L"\n\nPercorso eseguibile:\n");
        wcat(msg_buf, 2048, g_best.exe_path);
        wcat(msg_buf, 2048, L"\n\nArgomenti di avvio:\n--disable-gpu-compositing");
        wcat(msg_buf, 2048, extra_args);
        MessageBoxW(NULL, msg_buf, L"Cloudy - Stato Launcher", MB_OK | MB_ICONINFORMATION);
        ExitProcess(0);
    }

    if (opt_restart) {
        kill_existing_claude();
    }

    /* Build launch arguments */
    wcopy(shell_args, 1200, L"--disable-gpu-compositing");
    wcat(shell_args, 1200, extra_args);

    /* Attempt 1: Hidden cmd.exe /c start "" "<exe>" --disable-gpu-compositing
       (Exact equivalent of the working G-Sync fix command, with CREATE_NO_WINDOW so no CMD window flashes) */
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        memset(&pi, 0, sizeof(pi));

        wcopy(cmd_line, 2048, L"cmd.exe /c start \"\" \"");
        wcat(cmd_line, 2048, g_best.exe_path);
        wcat(cmd_line, 2048, L"\" ");
        wcat(cmd_line, 2048, shell_args);

        if (CreateProcessW(NULL, cmd_line, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, g_best.app_dir, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            ExitProcess(0);
        }
    }

    /* Attempt 2: Direct CreateProcessW on app\claude.exe */
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        memset(&pi, 0, sizeof(pi));

        wcopy(cmd_line, 2048, L"\"");
        wcat(cmd_line, 2048, g_best.exe_path);
        wcat(cmd_line, 2048, L"\" ");
        wcat(cmd_line, 2048, shell_args);

        if (CreateProcessW(g_best.exe_path, cmd_line, NULL, NULL, FALSE, 0, NULL, g_best.app_dir, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            ExitProcess(0);
        }
    }

    /* Attempt 3: ShellExecuteW fallback */
    if ((ULONG_PTR)ShellExecuteW(NULL, L"open", g_best.exe_path, shell_args, g_best.app_dir, SW_SHOWNORMAL) > 32) {
        ExitProcess(0);
    }

    MessageBoxW(
        NULL,
        L"Impossibile avviare Claude Desktop.\nVerifica i permessi dell'applicazione.",
        L"Cloudy - Errore di avvio",
        MB_OK | MB_ICONERROR
    );
    ExitProcess(1);
}

/* Ensure a non-empty .reloc section is emitted for Windows 10/11 ASLR compatibility */
void * volatile g_aslr_anchor = (void *)&WinMainCRTStartup;

