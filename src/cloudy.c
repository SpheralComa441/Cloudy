/*
 * Cloudy - Ultra-lightweight launcher for Claude Desktop on Windows (flicker workaround)
 *
 * Automatically finds the latest installed version of Claude Desktop
 * (both MSIX/WindowsApps packages like Claude_1.10628.2.0_x64__pzs8sxrjxfjjc
 * and standard AppData installations) without requiring Administrator privileges,
 * and launches app\claude.exe with the Chromium switch(es) of the selected mode:
 *   gpu   (default)  --disable-gpu-compositing
 *   dcomp            --disable-direct-composition   (needed on some RTX 50 series)
 *   both             both switches
 * The mode comes from --mode=<gpu|dcomp|both> / --dcomp, or from "mode=" in a
 * Cloudy.ini file next to Cloudy.exe.
 *
 * After launching, Cloudy stays in the background as an idle "guardian" (it just
 * waits on Claude's process handle, 0% CPU) until Claude is closed. If Claude
 * restarts itself after an auto-update, the updater relaunches it WITHOUT the
 * switches and the flicker comes back: the guardian notices they are missing and
 * relaunches the new version with them.
 * Use --no-watch to get the old "launch and exit immediately" behaviour.
 *
 * Zero dependencies, zero console window.
 */

typedef unsigned short WCHAR;
typedef unsigned short USHORT;
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
#define ERROR_ALREADY_EXISTS 183L
#define ERROR_MORE_DATA    234L

#define CREATE_NO_WINDOW   0x08000000
#define STARTF_USESHOWWINDOW 0x00000001
#define STARTF_USESTDHANDLES 0x00000100
#define SW_HIDE            0
#define SW_SHOWNORMAL      1
#define HANDLE_FLAG_INHERIT 0x00000001

#define INFINITE           0xFFFFFFFF
#define WAIT_OBJECT_0      0x00000000
#define WAIT_TIMEOUT       0x00000102
#define WAIT_ABANDONED     0x00000080

#define PROCESS_TERMINATE                 0x00000001
#define PROCESS_QUERY_LIMITED_INFORMATION 0x00001000
#define SYNCHRONIZE                       0x00100000
#define TH32CS_SNAPPROCESS                0x00000002
#define ProcessCommandLineInformation     60

#define MB_OK              0x00000000
#define MB_ICONERROR       0x00000010
#define MB_ICONWARNING     0x00000030
#define MB_ICONINFORMATION 0x00000040

#define GENERIC_READ       0x80000000
#define FILE_SHARE_READ    0x00000001
#define FILE_SHARE_WRITE   0x00000002
#define OPEN_EXISTING      3
#define FILE_ATTRIBUTE_NORMAL 0x00000080

/* Fix modes: which Chromium switches Claude is launched with */
#define FIX_GPU            1
#define FIX_DCOMP          2
#define FLAG_GPU           L"--disable-gpu-compositing"
#define FLAG_DCOMP         L"--disable-direct-composition"
#define CONFIG_NAME        L"Cloudy.ini"

#define GUARDIAN_MUTEX     L"Local\\Cloudy.Guardian"
#define GUARDIAN_STOP      L"Local\\Cloudy.Guardian.Stop"

/* Guardian tuning */
#define GUARD_POLL_MS      2000     /* poll interval while Claude is NOT running */
#define GUARD_GRACE_MS     180000   /* after Claude exits, wait this long for an updater relaunch */
#define GUARD_MAX_RESTARTS 3        /* consecutive fix attempts before giving up (avoid loops) */
#define KILL_TIMEOUT_MS    5000
#define GUARD_TAKEOVER_MS  15000    /* how long a new launch waits for the old guardian to step down */

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

typedef struct _PROCESSENTRY32W {
    DWORD dwSize;
    DWORD cntUsage;
    DWORD th32ProcessID;
    ULONG_PTR th32DefaultHeapID;
    DWORD th32ModuleID;
    DWORD cntThreads;
    DWORD th32ParentProcessID;
    long pcPriClassBase;
    DWORD dwFlags;
    WCHAR szExeFile[260];
} PROCESSENTRY32W;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    WCHAR *Buffer;
} UNICODE_STRING;

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
__declspec(dllimport) HANDLE __stdcall OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
__declspec(dllimport) BOOL __stdcall TerminateProcess(HANDLE hProcess, unsigned int uExitCode);
__declspec(dllimport) BOOL __stdcall QueryFullProcessImageNameW(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, DWORD *lpdwSize);
__declspec(dllimport) HANDLE __stdcall CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID);
__declspec(dllimport) BOOL __stdcall Process32FirstW(HANDLE hSnapshot, PROCESSENTRY32W *lppe);
__declspec(dllimport) BOOL __stdcall Process32NextW(HANDLE hSnapshot, PROCESSENTRY32W *lppe);
__declspec(dllimport) HANDLE __stdcall CreateMutexW(SECURITY_ATTRIBUTES *lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName);
__declspec(dllimport) HANDLE __stdcall OpenMutexW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
__declspec(dllimport) DWORD __stdcall GetLastError(void);
__declspec(dllimport) BOOL __stdcall ReleaseMutex(HANDLE hMutex);
__declspec(dllimport) HANDLE __stdcall CreateEventW(SECURITY_ATTRIBUTES *lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
__declspec(dllimport) BOOL __stdcall SetEvent(HANDLE hEvent);
__declspec(dllimport) BOOL __stdcall ResetEvent(HANDLE hEvent);
__declspec(dllimport) DWORD __stdcall WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
__declspec(dllimport) DWORD __stdcall GetTickCount(void);
__declspec(dllimport) DWORD __stdcall GetModuleFileNameW(HINSTANCE hModule, LPWSTR lpFilename, DWORD nSize);
__declspec(dllimport) HANDLE __stdcall CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

__declspec(dllimport) long __stdcall NtQueryInformationProcess(HANDLE ProcessHandle, int ProcessInformationClass, void *ProcessInformation, unsigned long ProcessInformationLength, unsigned long *ReturnLength);

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

/* Append one argument to a command line, quoted with the rules used by
   CommandLineToArgvW / the MSVC runtime (which Chromium/Electron also use).
   The argument is skipped entirely if it does not fit, instead of being
   truncated into an unbalanced quote. */
static void append_quoted_arg(LPWSTR dst, int cap, LPCWSTR arg) {
    int len = wlen(dst);
    int alen = wlen(arg);
    int i, n, bs = 0;

    if (len + 2 * alen + 3 >= cap) return;

    dst[len++] = L' ';
    dst[len++] = L'"';
    for (i = 0; i < alen; i++) {
        if (arg[i] == L'\\') {
            bs++;
            continue;
        }
        /* Backslashes are only special when followed by a quote */
        for (n = 0; n < (arg[i] == L'"' ? bs * 2 + 1 : bs); n++) dst[len++] = L'\\';
        dst[len++] = arg[i];
        bs = 0;
    }
    /* Trailing backslashes precede the closing quote: double them */
    for (n = 0; n < bs * 2; n++) dst[len++] = L'\\';
    dst[len++] = L'"';
    dst[len] = 0;
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

/* Fix switches of the selected mode + forwarded user arguments (already quoted) */
static WCHAR g_launch_args[2048];

/* ------------------------------------------------------------------------- */
/* Fix mode: --mode= / --dcomp on the command line, or mode= in Cloudy.ini   */
/* ------------------------------------------------------------------------- */

static int g_fix_mask = FIX_GPU;
static WCHAR g_fix_flags[128];      /* switches of g_fix_mask, space separated */
static LPCWSTR g_fix_source = L"predefinita";
static WCHAR g_config_path[1024];

static int parse_mode(LPCWSTR v) {
    if (weq_i(v, L"gpu")) return FIX_GPU;
    if (weq_i(v, L"dcomp")) return FIX_DCOMP;
    if (weq_i(v, L"both")) return FIX_GPU | FIX_DCOMP;
    return 0;
}

static LPCWSTR mode_name(int mask) {
    if (mask == FIX_DCOMP) return L"dcomp";
    if (mask == (FIX_GPU | FIX_DCOMP)) return L"both";
    return L"gpu";
}

static void build_fix_flags(void) {
    g_fix_flags[0] = 0;
    if (g_fix_mask & FIX_GPU) wcat(g_fix_flags, 128, FLAG_GPU);
    if (g_fix_mask & FIX_DCOMP) {
        if (g_fix_flags[0]) wcat(g_fix_flags, 128, L" ");
        wcat(g_fix_flags, 128, FLAG_DCOMP);
    }
}

/* Is `sw` present in `cmd` as a whole switch? --disable-direct-composition must not
   match --disable-direct-composition-video-overlays, for example. */
static int cmd_has_switch(LPCWSTR cmd, LPCWSTR sw) {
    int i, j, n = wlen(sw);
    WCHAR next;
    if (!cmd || n == 0) return 0;
    for (i = 0; cmd[i]; i++) {
        if (i > 0 && cmd[i - 1] != L' ' && cmd[i - 1] != L'\t' && cmd[i - 1] != L'"') continue;
        for (j = 0; j < n; j++) {
            if (!cmd[i + j] || wlower(cmd[i + j]) != wlower(sw[j])) break;
        }
        if (j < n) continue;
        next = cmd[i + n];
        if (next == 0 || next == L' ' || next == L'\t' || next == L'"' || next == L'=') return 1;
    }
    return 0;
}

/* 1 if `cmd` carries exactly the fix switches Cloudy launches Claude with (those of the
   selected mode, plus any the user forwarded explicitly). An extra switch counts as a
   mismatch too, so that switching e.g. from "both" to "gpu" really drops
   --disable-direct-composition. */
static int cmd_has_fix_flags(LPCWSTR cmd) {
    if (cmd_has_switch(cmd, FLAG_GPU) != cmd_has_switch(g_launch_args, FLAG_GPU)) return 0;
    if (cmd_has_switch(cmd, FLAG_DCOMP) != cmd_has_switch(g_launch_args, FLAG_DCOMP)) return 0;
    return 1;
}

static int is_space(WCHAR c) {
    return c == L' ' || c == L'\t' || c == L'\r';
}

/* Reads "mode=<gpu|dcomp|both>" from Cloudy.ini next to Cloudy.exe.
   Lines starting with ';' or '#' and [sections] are ignored; keys are case-insensitive.
   Returns the mode mask, 0 if the file or the key is missing, -1 if the value is invalid
   (the bad value is copied to bad_value). */
static int read_config_mode(LPWSTR bad_value, int bad_cap) {
    static BYTE raw[4096];
    static WCHAR text[4096];
    HANDLE h;
    DWORD n = 0, m;
    int i, len = 0, pos = 0, result = 0;

    m = GetModuleFileNameW(NULL, g_config_path, 1024);
    if (m == 0 || m >= 1024) {
        g_config_path[0] = 0;
        return 0;
    }
    for (i = (int)m; i > 0 && g_config_path[i - 1] != L'\\' && g_config_path[i - 1] != L'/'; i--) {
    }
    g_config_path[i] = 0;
    wcat(g_config_path, 1024, CONFIG_NAME);

    h = CreateFileW(g_config_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    if (!ReadFile(h, raw, sizeof(raw), &n, NULL)) n = 0;
    CloseHandle(h);

    /* Notepad may save UTF-16 LE ("Unicode") or UTF-8 with/without BOM; only ASCII matters */
    if (n >= 2 && raw[0] == 0xFF && raw[1] == 0xFE) {
        for (i = 2; i + 1 < (int)n && len < 4095; i += 2) text[len++] = (WCHAR)(raw[i] | (raw[i + 1] << 8));
    } else {
        i = (n >= 3 && raw[0] == 0xEF && raw[1] == 0xBB && raw[2] == 0xBF) ? 3 : 0;
        for (; i < (int)n && len < 4095; i++) text[len++] = (WCHAR)raw[i];
    }
    text[len] = 0;

    while (pos < len) {
        int line_end = pos, k0, k1, v0, v1, eq = -1;
        while (line_end < len && text[line_end] != L'\n') line_end++;

        k0 = pos;
        while (k0 < line_end && is_space(text[k0])) k0++;
        if (k0 < line_end && text[k0] != L';' && text[k0] != L'#' && text[k0] != L'[') {
            for (i = k0; i < line_end; i++) {
                if (text[i] == L'=') {
                    eq = i;
                    break;
                }
            }
        }
        if (eq > k0) {
            k1 = eq;
            while (k1 > k0 && is_space(text[k1 - 1])) k1--;
            v0 = eq + 1;
            while (v0 < line_end && is_space(text[v0])) v0++;
            v1 = v0;
            while (v1 < line_end && text[v1] != L';' && text[v1] != L'#') v1++; /* inline comment */
            while (v1 > v0 && is_space(text[v1 - 1])) v1--;

            if (k1 - k0 == 4 && wstarts_with_i(text + k0, L"mode")) {
                WCHAR saved = text[v1];
                text[v1] = 0;
                result = parse_mode(text + v0);
                if (!result) {
                    wcopy(bad_value, bad_cap, text + v0);
                    result = -1;
                }
                text[v1] = saved;
            }
        }
        pos = line_end + 1;
    }
    return result;
}

static void warn_invalid_mode(LPCWSTR where, LPCWSTR value) {
    static WCHAR msg[1024];
    wcopy(msg, 1024, L"Modalità non valida in ");
    wcat(msg, 1024, where);
    wcat(msg, 1024, L": \"");
    wcat(msg, 1024, value);
    wcat(msg, 1024, L"\"\n\nValori ammessi:\n"
                    L"• gpu = --disable-gpu-compositing (predefinita)\n"
                    L"• dcomp = --disable-direct-composition\n"
                    L"• both = entrambi\n\n"
                    L"Cloudy prosegue con la modalità ");
    wcat(msg, 1024, mode_name(g_fix_mask));
    wcat(msg, 1024, L".");
    MessageBoxW(NULL, msg, L"Cloudy - Configurazione", MB_OK | MB_ICONWARNING);
}

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
        if (res == ERROR_MORE_DATA) continue; /* name too long for our buffer: skip it, keep enumerating */
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

        /* Never block forever on a stuck PowerShell: once it has exited (or been
           killed) every write end of the pipe is closed and ReadFile cannot hang.
           The output is a single short line, far below the pipe buffer size. */
        if (WaitForSingleObject(pi.hProcess, 15000) == WAIT_TIMEOUT) {
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 2000);
        }

        while (total_read < sizeof(out_buf) - 1 &&
               ReadFile(hRead, out_buf + total_read, (DWORD)(sizeof(out_buf) - 1 - total_read), &bytes_read, NULL) &&
               bytes_read > 0) {
            total_read += bytes_read;
        }
        out_buf[total_read] = 0;

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

/* ------------------------------------------------------------------------- */
/* Running Claude Desktop processes                                          */
/* ------------------------------------------------------------------------- */

typedef struct {
    DWORD pid;
    DWORD ppid;
    int is_main; /* 1 = Electron main/browser process (its parent is not another Claude Desktop process) */
} ClaudeProc;

#define MAX_CLAUDE_PROCS 256
static ClaudeProc g_procs[MAX_CLAUDE_PROCS];
static int g_nprocs;

/* Only Claude *Desktop* binaries. The Claude Code CLI is also called claude.exe
   (e.g. %USERPROFILE%\.local\bin\claude.exe) and must never be killed by us. */
static int is_desktop_claude_image(LPCWSTR path) {
    int n;
    if (!path || !path[0]) return 0;
    if (wcontains_i(path, L"\\WindowsApps\\") &&
        (wcontains_i(path, L"\\Claude_") || wcontains_i(path, L"\\Anthropic.Claude_") || wcontains_i(path, L"pzs8sxrjxfjjc"))) {
        return 1;
    }
    if (wcontains_i(path, L"\\AnthropicClaude\\")) return 1;
    if (wcontains_i(path, L"\\Programs\\Claude\\claude.exe")) return 1;
    if (g_best.found) {
        n = wlen(g_best.app_dir);
        if (n > 0 && wstarts_with_i(path, g_best.app_dir) && path[n] == L'\\') return 1;
    }
    return 0;
}

static int handle_is_desktop_claude(HANDLE h) {
    static WCHAR image[1024];
    DWORD size = 1024;
    if (!QueryFullProcessImageNameW(h, 0, image, &size)) return 0;
    return is_desktop_claude_image(image);
}

/* Returns the command line of a process (static buffer, overwritten by the next call), or NULL */
static WCHAR *read_command_line(DWORD pid) {
    /* UNICODE_STRING header followed by the string itself (max 32767 chars) */
    static unsigned long long buf[(65536 + 64) / 8];
    unsigned long ret_len = 0;
    UNICODE_STRING *us = (UNICODE_STRING *)buf;
    BYTE *buf_end = (BYTE *)buf + sizeof(buf);
    WCHAR *end;
    HANDLE h;
    long status;

    h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return NULL;

    /* Windows 8.1+. Leave room for a terminating NUL. */
    us->Buffer = NULL;
    status = NtQueryInformationProcess(h, ProcessCommandLineInformation, buf, (unsigned long)(sizeof(buf) - sizeof(WCHAR)), &ret_len);
    CloseHandle(h);
    if (status < 0 || !us->Buffer) return NULL;

    end = us->Buffer + us->Length / sizeof(WCHAR);
    if ((BYTE *)us->Buffer < (BYTE *)buf || (BYTE *)end + sizeof(WCHAR) > buf_end) return NULL;
    *end = 0;
    return us->Buffer;
}

/* 1 = command line has every switch of the current mode, 0 = it does not, -1 = unknown */
static int process_has_fix_flags(DWORD pid) {
    WCHAR *cmd = read_command_line(pid);
    if (!cmd) return -1;
    return cmd_has_fix_flags(cmd);
}

static void scan_claude_processes(void) {
    static PROCESSENTRY32W pe;
    HANDLE snap;
    WCHAR *cmd;
    int i, j;

    g_nprocs = 0;
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (g_nprocs < MAX_CLAUDE_PROCS && weq_i(pe.szExeFile, L"claude.exe")) {
                HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pe.th32ProcessID);
                if (h) {
                    /* Skip processes that already exited (zombies still referenced by a handle) */
                    if (WaitForSingleObject(h, 0) == WAIT_TIMEOUT && handle_is_desktop_claude(h)) {
                        g_procs[g_nprocs].pid = pe.th32ProcessID;
                        g_procs[g_nprocs].ppid = pe.th32ParentProcessID;
                        g_procs[g_nprocs].is_main = 1;
                        g_nprocs++;
                    }
                    CloseHandle(h);
                }
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    /* Renderer/GPU/utility/crashpad helpers are children of the main process */
    for (i = 0; i < g_nprocs; i++) {
        for (j = 0; j < g_nprocs; j++) {
            if (i != j && g_procs[i].ppid == g_procs[j].pid) {
                g_procs[i].is_main = 0;
                break;
            }
        }
    }

    /* A helper can outlive its main process (e.g. crashpad-handler): Chromium marks
       every helper with --type=..., the main process never has it. */
    for (i = 0; i < g_nprocs; i++) {
        if (!g_procs[i].is_main) continue;
        cmd = read_command_line(g_procs[i].pid);
        if (cmd && cmd_has_switch(cmd, L"--type")) g_procs[i].is_main = 0;
    }
}

/* 1 = at least one main Claude process runs with fix switches other than ours
   (none at all after an auto-update, or those of a different mode) */
static int any_main_needs_restart(void) {
    int i;
    for (i = 0; i < g_nprocs; i++) {
        if (g_procs[i].is_main && process_has_fix_flags(g_procs[i].pid) == 0) return 1;
    }
    return 0;
}

/* Terminate every Claude Desktop process (main first, then helpers) and wait for them to exit,
   so that Electron's single-instance lock is released before we relaunch. */
static void kill_desktop_claude(void) {
    static HANDLE handles[MAX_CLAUDE_PROCS];
    int i, pass, n = 0;
    DWORD start, elapsed;

    scan_claude_processes();
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < g_nprocs; i++) {
            HANDLE h;
            if ((pass == 0) != (g_procs[i].is_main != 0)) continue;
            h = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, g_procs[i].pid);
            if (!h) continue;
            /* Re-check the image in case the PID was recycled since the snapshot */
            if (!handle_is_desktop_claude(h)) {
                CloseHandle(h);
                continue;
            }
            TerminateProcess(h, 1);
            handles[n++] = h;
        }
    }

    start = GetTickCount();
    for (i = 0; i < n; i++) {
        elapsed = GetTickCount() - start;
        WaitForSingleObject(handles[i], elapsed < KILL_TIMEOUT_MS ? KILL_TIMEOUT_MS - elapsed : 0);
        CloseHandle(handles[i]);
    }
    if (n > 0) Sleep(300);
}

static int launch_claude(void) {
    static WCHAR cmd_line[4096];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    /* Attempt 1: direct CreateProcessW (no cmd.exe in between, so no cmd quoting/metacharacter issues) */
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    wcopy(cmd_line, 4096, L"\"");
    wcat(cmd_line, 4096, g_best.exe_path);
    wcat(cmd_line, 4096, L"\" ");
    wcat(cmd_line, 4096, g_launch_args);

    if (CreateProcessW(g_best.exe_path, cmd_line, NULL, NULL, FALSE, 0, NULL, g_best.app_dir, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 1;
    }

    /* Attempt 2: ShellExecuteW (same mechanism used by `cmd /c start`) */
    if ((ULONG_PTR)ShellExecuteW(NULL, L"open", g_best.exe_path, g_launch_args, g_best.app_dir, SW_SHOWNORMAL) > 32) {
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Guardian: keeps the flag on after Claude auto-updates and restarts itself  */
/* ------------------------------------------------------------------------- */

/* Only one guardian runs at a time, and the most recent launch wins: a running guardian
   keeps the mode it was started with, so if it stayed in charge after the user switched
   mode it would "fix" Claude back to the old mode. Ownership of GUARDIAN_MUTEX is the
   guardian role; GUARDIAN_STOP asks the current owner to step down. */
static HANDLE g_guard_mutex;
static HANDLE g_stop_event;

static int stop_requested(DWORD wait_ms) {
    return g_stop_event && WaitForSingleObject(g_stop_event, wait_ms) == WAIT_OBJECT_0;
}

/* Returns 1 if this process now holds the guardian role */
static int take_over_guardian(void) {
    DWORD w;

    g_stop_event = CreateEventW(NULL, TRUE, FALSE, GUARDIAN_STOP);
    g_guard_mutex = CreateMutexW(NULL, FALSE, GUARDIAN_MUTEX);
    if (!g_stop_event || !g_guard_mutex) return 0;

    w = WaitForSingleObject(g_guard_mutex, 0);
    if (w != WAIT_OBJECT_0 && w != WAIT_ABANDONED) {
        /* Another Cloudy is guarding Claude: ask it to step down and take its place */
        SetEvent(g_stop_event);
        w = WaitForSingleObject(g_guard_mutex, GUARD_TAKEOVER_MS);
    }
    if (w != WAIT_OBJECT_0 && w != WAIT_ABANDONED) return 0;
    ResetEvent(g_stop_event);
    return 1;
}

static void release_guardian(void) {
    if (g_guard_mutex) {
        ReleaseMutex(g_guard_mutex);
        CloseHandle(g_guard_mutex);
        g_guard_mutex = NULL;
    }
}

static void run_guardian(void) {
    DWORD idle_ms = 0;
    int restarts = 0;
    int i, main_idx, flag;

    for (;;) {
        if (stop_requested(0)) break; /* a newer Cloudy launch took over */
        scan_claude_processes();

        if (any_main_needs_restart()) {
            /* Claude was (re)started without our switches: typically the auto-updater
               relaunching the new version. Relaunch it with them. */
            if (restarts >= GUARD_MAX_RESTARTS) {
                static WCHAR msg[512];
                wcopy(msg, 512, L"Claude Desktop continua a ripartire senza ");
                wcat(msg, 512, g_fix_flags);
                wcat(msg, 512, L".\n\n"
                               L"Cloudy ha smesso di riavviarlo automaticamente per evitare un ciclo.\n"
                               L"Chiudi Claude e riaprilo con Cloudy (oppure usa Cloudy.exe --restart).");
                MessageBoxW(NULL, msg, L"Cloudy - Guardiano", MB_OK | MB_ICONWARNING);
                break;
            }
            restarts++;
            kill_desktop_claude();
            find_latest_claude(); /* the update moved Claude to a new versioned folder */
            if (!g_best.found || !launch_claude()) break;
            idle_ms = 0;
            if (stop_requested(GUARD_POLL_MS)) break;
            continue;
        }

        main_idx = -1;
        flag = -1;
        for (i = 0; i < g_nprocs; i++) {
            if (g_procs[i].is_main) {
                main_idx = i;
                flag = process_has_fix_flags(g_procs[i].pid);
                break;
            }
        }

        if (main_idx >= 0) {
            HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, g_procs[main_idx].pid);
            if (flag == 1) restarts = 0;
            idle_ms = 0;
            if (h) {
                /* Zero CPU while Claude runs: wait for the main process to exit
                   (or for a newer Cloudy launch to take over) */
                HANDLE waits[2];
                DWORD w;
                waits[0] = h;
                waits[1] = g_stop_event;
                w = WaitForMultipleObjects(g_stop_event ? 2 : 1, waits, FALSE, INFINITE);
                CloseHandle(h);
                if (w == WAIT_OBJECT_0 + 1) break;
                continue;
            }
        } else if (idle_ms >= GUARD_GRACE_MS) {
            /* Claude was closed and nobody relaunched it: we're done */
            break;
        }

        if (stop_requested(GUARD_POLL_MS)) break;
        idle_ms += GUARD_POLL_MS;
    }
}

static void show_status(void) {
    static WCHAR msg_buf[4096];
    static WCHAR num[16];
    HANDLE mutex;
    int i, shown = 0, flag;

    wcopy(msg_buf, 4096, L"Claude Desktop rilevato correttamente!\n\nVersione rilevata: ");
    wcat(msg_buf, 4096, g_best.version_str);
    wcat(msg_buf, 4096, L"\n\nPercorso eseguibile:\n");
    wcat(msg_buf, 4096, g_best.exe_path);
    wcat(msg_buf, 4096, L"\n\nModalità: ");
    wcat(msg_buf, 4096, mode_name(g_fix_mask));
    wcat(msg_buf, 4096, L" (");
    wcat(msg_buf, 4096, g_fix_source);
    wcat(msg_buf, 4096, L")\nFile di configurazione: ");
    wcat(msg_buf, 4096, g_config_path[0] ? g_config_path : L"(non determinabile)");
    wcat(msg_buf, 4096, L"\n\nArgomenti di avvio:\n");
    wcat(msg_buf, 4096, g_launch_args);

    wcat(msg_buf, 4096, L"\n\nIstanza in esecuzione: ");
    scan_claude_processes();
    for (i = 0; i < g_nprocs; i++) {
        if (!g_procs[i].is_main) continue;
        if (shown++) wcat(msg_buf, 4096, L", ");
        wcat(msg_buf, 4096, L"PID ");
        uint_to_wstr(g_procs[i].pid, num, 16);
        wcat(msg_buf, 4096, num);
        flag = process_has_fix_flags(g_procs[i].pid);
        if (flag == 1) {
            wcat(msg_buf, 4096, L" (con ");
            wcat(msg_buf, 4096, g_fix_flags);
            wcat(msg_buf, 4096, L")");
        } else if (flag == 0) {
            wcat(msg_buf, 4096, L" (NON allineata: deve avere solo ");
            wcat(msg_buf, 4096, g_fix_flags);
            wcat(msg_buf, 4096, L"; avvia Cloudy per correggerla)");
        } else {
            wcat(msg_buf, 4096, L" (flag non verificabili)");
        }
    }
    if (!shown) wcat(msg_buf, 4096, L"nessuna");

    wcat(msg_buf, 4096, L"\nGuardiano aggiornamenti: ");
    mutex = OpenMutexW(SYNCHRONIZE, FALSE, GUARDIAN_MUTEX);
    if (mutex) {
        wcat(msg_buf, 4096, L"attivo");
        CloseHandle(mutex);
    } else {
        wcat(msg_buf, 4096, L"non attivo");
    }

    MessageBoxW(NULL, msg_buf, L"Cloudy - Stato Launcher", MB_OK | MB_ICONINFORMATION);
}

void __stdcall WinMainCRTStartup(void) {
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    int opt_status = 0;
    int opt_restart = 0;
    int opt_no_watch = 0;
    int cli_mask = 0, cfg_mask, i, is_guardian;
    LPCWSTR bad_cli_mode = NULL;
    static WCHAR bad_cfg_mode[64];

    /* Pass 1: fix mode from the command line (it overrides Cloudy.ini) */
    if (argv) {
        for (i = 1; i < argc; i++) {
            if (weq_i(argv[i], L"--dcomp") || weq_i(argv[i], L"/dcomp")) {
                cli_mask = FIX_DCOMP;
            } else if (wstarts_with_i(argv[i], L"--mode=")) {
                int m = parse_mode(argv[i] + 7);
                if (m) cli_mask = m;
                else bad_cli_mode = argv[i] + 7;
            }
        }
    }

    cfg_mask = read_config_mode(bad_cfg_mode, 64);
    if (cfg_mask > 0) {
        g_fix_mask = cfg_mask;
        g_fix_source = CONFIG_NAME;
    }
    if (cli_mask) {
        g_fix_mask = cli_mask;
        g_fix_source = L"riga di comando";
    }
    if (cfg_mask < 0 && !cli_mask) warn_invalid_mode(CONFIG_NAME, bad_cfg_mode);
    if (bad_cli_mode) warn_invalid_mode(L"--mode", bad_cli_mode);
    build_fix_flags();

    /* Pass 2: Cloudy options; everything else is forwarded to Claude */
    wcopy(g_launch_args, 2048, g_fix_flags);
    if (argv) {
        for (i = 1; i < argc; i++) {
            if (weq_i(argv[i], L"--status") || weq_i(argv[i], L"/status")) {
                opt_status = 1;
            } else if (weq_i(argv[i], L"--restart") || weq_i(argv[i], L"/restart")) {
                opt_restart = 1;
            } else if (weq_i(argv[i], L"--no-watch") || weq_i(argv[i], L"/nowatch")) {
                opt_no_watch = 1;
            } else if (weq_i(argv[i], L"--dcomp") || weq_i(argv[i], L"/dcomp") || wstarts_with_i(argv[i], L"--mode=")) {
                /* already handled */
            } else if (!((g_fix_mask & FIX_GPU) && weq_i(argv[i], FLAG_GPU)) &&
                       !((g_fix_mask & FIX_DCOMP) && weq_i(argv[i], FLAG_DCOMP))) {
                append_quoted_arg(g_launch_args, 2048, argv[i]);
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
        show_status();
        ExitProcess(0);
    }

    /* Become the (only) guardian before touching Claude, so that a guardian started
       earlier, possibly in another mode, cannot undo what this launch does. */
    is_guardian = take_over_guardian();

    /* If Claude is already running without the flag (started by the auto-updater,
       by "launch at login", from the Start menu...), Electron's single-instance lock
       would just forward our launch to that flickering instance. Restart it instead. */
    scan_claude_processes();
    if (g_nprocs > 0 && (opt_restart || any_main_needs_restart())) {
        kill_desktop_claude();
    }

    if (!launch_claude()) {
        MessageBoxW(
            NULL,
            L"Impossibile avviare Claude Desktop.\nVerifica i permessi dell'applicazione.",
            L"Cloudy - Errore di avvio",
            MB_OK | MB_ICONERROR
        );
        release_guardian();
        ExitProcess(1);
    }

    if (!opt_no_watch && is_guardian) {
        run_guardian();
    }
    release_guardian();
    ExitProcess(0);
}

/* Ensure a non-empty .reloc section is emitted for Windows 10/11 ASLR compatibility */
void * volatile g_aslr_anchor = (void *)&WinMainCRTStartup;
