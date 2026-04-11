#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <shellapi.h>
#include <wchar.h>
#include <math.h>

#ifdef _MSC_VER
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "advapi32.lib")
#endif

/* DPI 感知 (消除缩放模糊) */
typedef enum {
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS_ENUM;

static void EnableDPIAwareness(void) {
    /* 优先尝试 Per-Monitor V2 (Win10 1703+) */
    typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        PFN_SetProcessDpiAwarenessContext pfn =
            (PFN_SetProcessDpiAwarenessContext)GetProcAddress(hUser, "SetProcessDpiAwarenessContext");
        if (pfn) {
            /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4 */
            if (pfn((HANDLE)(LONG_PTR)-4)) return;
        }
    }
    /* 回退: SetProcessDpiAwareness (Win8.1+) */
    typedef HRESULT (WINAPI *PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS_ENUM);
    HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore) {
        PFN_SetProcessDpiAwareness pfn2 =
            (PFN_SetProcessDpiAwareness)GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (pfn2) pfn2(PROCESS_PER_MONITOR_DPI_AWARE);
        FreeLibrary(hShcore);
        return;
    }
    /* 最终回退: SetProcessDPIAware (Vista+) */
    if (hUser) {
        typedef BOOL (WINAPI *PFN_SetProcessDPIAware)(void);
        PFN_SetProcessDPIAware pfn3 =
            (PFN_SetProcessDPIAware)GetProcAddress(hUser, "SetProcessDPIAware");
        if (pfn3) pfn3();
    }
}

#define IDI_APP 1

/* ══════════════════ 可调参数 (96 DPI 基准) ══════════════════ */
#define BASE_SINGLE_W    120     /* 单设备卡片宽度 */
#define BASE_DUAL_W      240     /* 双设备卡片宽度 (M4 方案) */
#define BASE_HEIGHT      110     /* 卡片高度 */
#define BASE_CORNER       18.0f  /* CapsBar 风格大圆角 */
#define BASE_BOTTOM_GAP   50
#define BASE_RIGHT_GAP    16

#define BASE_ICON_CY      42.0f  /* 图标中心 Y */
#define BASE_TEXT_Y        80.0f  /* 文字区域顶部 Y */
#define BASE_TEXT_H        24.0f  /* 文字区域高度 */

/* DPI 缩放后的实际值 (运行时设置) */
static float g_dpiScale    = 1.0f;
static int   g_osdSingleW  = BASE_SINGLE_W;
static int   g_osdDualW    = BASE_DUAL_W;
static int   g_osdHeight   = BASE_HEIGHT;
static float g_osdCorner   = BASE_CORNER;
static int   g_bottomGap   = BASE_BOTTOM_GAP;
static int   g_rightGap    = BASE_RIGHT_GAP;
static float g_iconCY      = BASE_ICON_CY;
static float g_textY       = BASE_TEXT_Y;
static float g_textH       = BASE_TEXT_H;

static void ApplyDPIScale(void) {
    typedef UINT (WINAPI *PFN_GetDpiForSystem)(void);
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    UINT dpi = 96;
    if (hUser) {
        PFN_GetDpiForSystem pfn =
            (PFN_GetDpiForSystem)GetProcAddress(hUser, "GetDpiForSystem");
        if (pfn) dpi = pfn();
    }
    g_dpiScale   = (float)dpi / 96.0f;
    g_osdSingleW = (int)(BASE_SINGLE_W * g_dpiScale + 0.5f);
    g_osdDualW   = (int)(BASE_DUAL_W   * g_dpiScale + 0.5f);
    g_osdHeight  = (int)(BASE_HEIGHT    * g_dpiScale + 0.5f);
    g_osdCorner  = BASE_CORNER  * g_dpiScale;
    g_bottomGap  = (int)(BASE_BOTTOM_GAP * g_dpiScale + 0.5f);
    g_rightGap   = (int)(BASE_RIGHT_GAP  * g_dpiScale + 0.5f);
    g_iconCY     = BASE_ICON_CY * g_dpiScale;
    g_textY      = BASE_TEXT_Y  * g_dpiScale;
    g_textH      = BASE_TEXT_H  * g_dpiScale;
}

#define ANIM_INTERVAL     30      /* 动画帧间隔 ~33fps */
#define POLL_INTERVAL    800      /* 注册表轮询间隔 */
#define FADE_STEP_VAL     25      /* 每帧淡入/淡出步进 */



/* 麦克风填充动画参数 (方案C) */
#define MIC_FILL_SPEED     0.04f   /* 每帧相位增量 */

/* 四角取景框参数 */
#define CORNER_LEN         9.0f
#define CORNER_GAP         5.0f
#define CORNER_SPEED       0.094f  /* 每帧角度增量 (~2s周期) */

/* ═══ 暗色主题配色 · 方案5 暖焰橙 ═══ */
#define DARK_BG          0xF5281610u    /* rgba(40,22,16, 0.96) 深橙渐变 */
#define DARK_BORDER      0x4DFB923Cu    /* rgba(251,146,60, 0.3) 橙色边框 */
#define DARK_ICON        0xFFF0D0A8u    /* #f0d0a8 暖白偏橙 */
#define DARK_TEXT        0xFFF8E0C8u    /* #f8e0c8 暖白色 */
#define DARK_ANIM_RGB    0x00FB923Cu    /* 橙色 (无alpha) */

/* ═══ 亮色主题配色 · L2 毛玻璃白 ═══ */
#define LITE_BG          0xE0FFFFFFu    /* rgba(255,255,255, 0.88) */
#define LITE_BORDER      0x12000000u    /* rgba(0,0,0, 0.07) */
#define LITE_ICON        0xFF444444u    /* #444 */
#define LITE_TEXT        0xFF2D2D35u    /* #2d2d35 */
#define LITE_ANIM_RGB    0x00DC2626u    /* 红色 */

/* Timer IDs */
#define TIMER_ANIM   1
#define TIMER_POLL   2

/* Tray & Menu */
#define WM_TRAYICON      (WM_USER + 1)
#define IDM_EXIT         1001
#define IDM_AUTOSTART    1002
#define IDM_THEME_AUTO   1010
#define IDM_THEME_DARK   1011
#define IDM_THEME_LIGHT  1012
#define IDM_LANG_AUTO    1020
#define IDM_LANG_ZH      1021
#define IDM_LANG_EN      1022
#define IDM_PREVIEW      1030

#define MAX_APP_NAME     64
#define MAX_APPS          8

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ══════════ GDI+ Flat C API ══════════ */
typedef int GpStatus;
typedef struct { UINT32 GdiplusVersion; void *DebugEventCallback;
                 BOOL SuppressBackgroundThread; BOOL SuppressExternalCodecs;
} GdiplusStartupInput;
typedef struct { float X, Y, Width, Height; } GpRectF;
typedef void *GpGraphics, *GpSolidFill, *GpPen, *GpPath,
             *GpFont, *GpFontFamily, *GpStringFormat;

#define SmoothingModeAntiAlias            4
#define TextRenderingHintAntiAlias        4
#define TextRenderingHintAntiAliasGridFit 3
#define UnitPixel                         2
#define FillModeAlternate                 0
#define StringAlignmentNear               0
#define StringAlignmentCenter             1

extern GpStatus __stdcall GdiplusStartup(ULONG_PTR*, const GdiplusStartupInput*, void*);
extern void     __stdcall GdiplusShutdown(ULONG_PTR);
extern GpStatus __stdcall GdipCreateFromHDC(HDC, GpGraphics**);
extern GpStatus __stdcall GdipDeleteGraphics(GpGraphics*);
extern GpStatus __stdcall GdipGraphicsClear(GpGraphics*, DWORD);
extern GpStatus __stdcall GdipSetSmoothingMode(GpGraphics*, int);
extern GpStatus __stdcall GdipSetTextRenderingHint(GpGraphics*, int);
extern GpStatus __stdcall GdipCreateSolidFill(DWORD, GpSolidFill**);
extern GpStatus __stdcall GdipDeleteBrush(GpSolidFill*);
extern GpStatus __stdcall GdipCreatePen1(DWORD, float, int, GpPen**);
extern GpStatus __stdcall GdipDeletePen(GpPen*);
extern GpStatus __stdcall GdipCreatePath(int, GpPath**);
extern GpStatus __stdcall GdipDeletePath(GpPath*);
extern GpStatus __stdcall GdipAddPathArc(GpPath*, float, float, float, float, float, float);
extern GpStatus __stdcall GdipClosePathFigure(GpPath*);
extern GpStatus __stdcall GdipFillPath(GpGraphics*, GpSolidFill*, GpPath*);
extern GpStatus __stdcall GdipDrawPath(GpGraphics*, GpPen*, GpPath*);
extern GpStatus __stdcall GdipCreateFontFamilyFromName(const WCHAR*, void*, GpFontFamily**);
extern GpStatus __stdcall GdipDeleteFontFamily(GpFontFamily*);
extern GpStatus __stdcall GdipCreateFont(GpFontFamily*, float, int, int, GpFont**);
extern GpStatus __stdcall GdipDeleteFont(GpFont*);
extern GpStatus __stdcall GdipCreateStringFormat(int, LANGID, GpStringFormat**);
extern GpStatus __stdcall GdipDeleteStringFormat(GpStringFormat*);
extern GpStatus __stdcall GdipSetStringFormatAlign(GpStringFormat*, int);
extern GpStatus __stdcall GdipSetStringFormatLineAlign(GpStringFormat*, int);
extern GpStatus __stdcall GdipDrawString(GpGraphics*, const WCHAR*, int, GpFont*,
                                         const GpRectF*, GpStringFormat*, GpSolidFill*);
extern GpStatus __stdcall GdipDrawLine(GpGraphics*, GpPen*, float, float, float, float);
extern GpStatus __stdcall GdipDrawArc(GpGraphics*, GpPen*, float, float, float, float, float, float);
extern GpStatus __stdcall GdipAddPathLine(GpPath*, float, float, float, float);

/* ══════════ 数据结构 ══════════ */
typedef struct {
    BOOL  active;
    int   appCount;
    WCHAR appNames[MAX_APPS][MAX_APP_NAME];
    WCHAR displayText[600];            /* 逗号分隔的应用名 */
} DeviceState;

/* ══════════ 多语言 ══════════ */
typedef struct {
    const WCHAR *tip;
    const WCHAR *menuAutostart;
    const WCHAR *menuTheme;
    const WCHAR *menuThemeAuto;
    const WCHAR *menuThemeDark;
    const WCHAR *menuThemeLight;
    const WCHAR *menuLang;
    const WCHAR *menuLangAuto;
    const WCHAR *menuLangZH;
    const WCHAR *menuLangEN;
    const WCHAR *menuExit;
    const WCHAR *menuPreview;
} LangStrings;

static const LangStrings g_langZH = {
    L"OnAir - \x9EA6\x514B\x98CE/\x6444\x50CF\x5934\x76D1\x89C6\x5668",
    L"\x5F00\x673A\x542F\x52A8",
    L"\x4E3B\x9898",
    L"\x8DDF\x968F\x7CFB\x7EDF",
    L"\x6697\x8272\x6A21\x5F0F",
    L"\x4EAE\x8272\x6A21\x5F0F",
    L"\x8BED\x8A00",
    L"\x8DDF\x968F\x7CFB\x7EDF",
    L"\x4E2D\x6587",
    L"English",
    L"\x9000\x51FA OnAir",
    L"\x9884\x89C8 OSD"
};

static const LangStrings g_langEN = {
    L"OnAir - Mic/Camera Monitor",
    L"Start with Windows",
    L"Theme",
    L"Follow System",
    L"Dark Mode",
    L"Light Mode",
    L"Language",
    L"Follow System",
    L"\x4E2D\x6587",
    L"English",
    L"Exit OnAir",
    L"Preview OSD"
};

static const LangStrings *g_lang = &g_langEN;

/* ══════════ 全局变量 ══════════ */
static HWND      g_hwndOSD    = NULL;
static HINSTANCE g_hInst      = NULL;
static ULONG_PTR g_gdipToken  = 0;
static HICON     g_iconIdle   = NULL;
static HICON     g_iconActive = NULL;
static NOTIFYICONDATAW g_nid  = {0};
static UINT WM_TASKBARCREATED = 0;

/* OSD 状态 */
static BYTE  g_alpha      = 0;
static BOOL  g_visible    = FALSE;
static BOOL  g_fadingIn   = FALSE;
static BOOL  g_fadingOut  = FALSE;
static BOOL  g_darkMode   = TRUE;

/* OSD 位图缓存 (持久分配) */
static HDC     g_cacheDC     = NULL;
static HBITMAP g_cacheBmp    = NULL;
static HBITMAP g_cacheOldBmp = NULL;

/* 设备状态 */
static DeviceState g_micState = {0};
static DeviceState g_camState = {0};
static BOOL g_prevMicActive = FALSE;
static BOOL g_prevCamActive = FALSE;
static int  g_previewState  = 0;  /* 0=关闭, 1=仅麦克风, 2=仅摄像头, 3=双设备 */


/* 麦克风填充动画 */
static float g_micFillPhase = 0.0f;

/* 四角取景框动画状态 */
static float g_cornerAngle = 0.0f;

/* GDI+ 缓存对象 */
static GpFontFamily   *g_ffUI     = NULL;
static GpFontFamily   *g_ffIcon   = NULL;   /* Segoe MDL2 Assets (图标字体) */
static GpFont         *g_fontText = NULL;
static GpFont         *g_fontIcon = NULL;    /* 图标字体 32px */
static GpStringFormat *g_sfCenter = NULL;

/* 配置 */
static int   g_themeMode = 0;   /* 0=跟随系统, 1=暗色, 2=亮色 */
static int   g_langMode  = 0;   /* 0=跟随系统, 1=中文, 2=英文 */
static WCHAR g_iniPath[MAX_PATH] = {0};

/* 前向声明 */
static void RenderOSD(void);
static void CompositeOSD(void);
static void ShowOSDFadeIn(void);
static void HideOSDFadeOut(void);
static LRESULT CALLBACK OSDWndProc(HWND, UINT, WPARAM, LPARAM);

/* ══════════ 配置文件 ══════════ */
static void InitConfigPath(void) {
    GetModuleFileNameW(NULL, g_iniPath, MAX_PATH);
    WCHAR *dot = wcsrchr(g_iniPath, L'.');
    if (dot) lstrcpyW(dot, L".ini");
}

static void LoadConfig(void) {
    g_themeMode = GetPrivateProfileIntW(L"Settings", L"ThemeMode", 0, g_iniPath);
    if (g_themeMode < 0 || g_themeMode > 2) g_themeMode = 0;
    g_langMode = GetPrivateProfileIntW(L"Settings", L"LangMode", 0, g_iniPath);
    if (g_langMode < 0 || g_langMode > 2) g_langMode = 0;
}

static void SaveConfig(void) {
    WCHAR val[4];
    wsprintfW(val, L"%d", g_themeMode);
    WritePrivateProfileStringW(L"Settings", L"ThemeMode", val, g_iniPath);
    wsprintfW(val, L"%d", g_langMode);
    WritePrivateProfileStringW(L"Settings", L"LangMode", val, g_iniPath);
}

/* ══════════ 语言 ══════════ */
static void ApplyLanguage(void) {
    if (g_langMode == 1) { g_lang = &g_langZH; return; }
    if (g_langMode == 2) { g_lang = &g_langEN; return; }
    LANGID lid = GetUserDefaultUILanguage();
    g_lang = ((lid & 0x3FF) == LANG_CHINESE) ? &g_langZH : &g_langEN;
}

/* ══════════ 主题 ══════════ */
static BOOL IsSystemDarkMode(void) {
    DWORD value = 1, size = sizeof(DWORD);
    RegGetValueW(HKEY_CURRENT_USER,
                 L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme", RRF_RT_DWORD, NULL, &value, &size);
    return value == 0;
}

static BOOL GetEffectiveDarkMode(void) {
    if (g_themeMode == 1) return TRUE;
    if (g_themeMode == 2) return FALSE;
    return IsSystemDarkMode();
}

/* ══════════ 开机启动 ══════════ */
static const WCHAR *REG_RUN_KEY  = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
static const WCHAR *REG_APP_NAME = L"OnAir";

static BOOL IsAutoStartEnabled(void) {
    HKEY hk;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_READ, &hk) != ERROR_SUCCESS)
        return FALSE;
    BOOL ok = (RegQueryValueExW(hk, REG_APP_NAME, NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
    RegCloseKey(hk);
    return ok;
}

static void SetAutoStart(BOOL enable) {
    HKEY hk;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_WRITE, &hk) != ERROR_SUCCESS)
        return;
    if (enable) {
        WCHAR path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        RegSetValueExW(hk, REG_APP_NAME, 0, REG_SZ,
                       (const BYTE *)path, (DWORD)((lstrlenW(path) + 1) * sizeof(WCHAR)));
    } else {
        RegDeleteValueW(hk, REG_APP_NAME);
    }
    RegCloseKey(hk);
}

/* ══════════ 注册表轮询 ══════════ */

static void ExtractAppName(const WCHAR *keyName, BOOL isNonPackaged, WCHAR *out) {
    if (isNonPackaged) {
        /* NonPackaged 键名: "C:#Path#To#app.exe" */
        const WCHAR *last = wcsrchr(keyName, L'#');
        if (last)
            lstrcpynW(out, last + 1, MAX_APP_NAME);
        else
            lstrcpynW(out, keyName, MAX_APP_NAME);
        /* 去掉 .exe */
        WCHAR *dot = wcsrchr(out, L'.');
        if (dot && _wcsicmp(dot, L".exe") == 0) *dot = L'\0';
    } else {
        /* 打包应用: "Microsoft.SkypeApp_xxxx" */
        lstrcpynW(out, keyName, MAX_APP_NAME);
        /* 去掉 _ 后缀 */
        WCHAR *under = wcschr(out, L'_');
        if (under) *under = L'\0';
        /* 取最后一段 (去掉 "Microsoft." 等前缀) */
        WCHAR *lastDot = wcsrchr(out, L'.');
        if (lastDot && lastDot != out) {
            size_t len = wcslen(lastDot + 1);
            wmemmove(out, lastDot + 1, len + 1);
        }
    }
}

static void BuildDisplayText(DeviceState *state) {
    state->displayText[0] = L'\0';
    size_t maxLen = (sizeof(state->displayText) / sizeof(WCHAR)) - 1;
    size_t pos = 0;
    for (int i = 0; i < state->appCount; i++) {
        if (i > 0) {
            if (pos + 2 >= maxLen) break;
            state->displayText[pos++] = L',';
            state->displayText[pos++] = L' ';
        }
        size_t nameLen = wcslen(state->appNames[i]);
        if (pos + nameLen > maxLen) nameLen = maxLen - pos;
        if (nameLen == 0) break;
        wmemcpy(state->displayText + pos, state->appNames[i], nameLen);
        pos += nameLen;
    }
    state->displayText[pos] = L'\0';
}

static void CheckDeviceKeys(HKEY hParent, DeviceState *state, BOOL isNonPackaged) {
    DWORD index = 0;
    WCHAR name[256];
    DWORD nameLen;

    for (;;) {
        nameLen = 256;
        if (RegEnumKeyExW(hParent, index++, name, &nameLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        if (!isNonPackaged && wcscmp(name, L"NonPackaged") == 0) {
            HKEY hNP;
            if (RegOpenKeyExW(hParent, L"NonPackaged", 0, KEY_READ, &hNP) == ERROR_SUCCESS) {
                CheckDeviceKeys(hNP, state, TRUE);
                RegCloseKey(hNP);
            }
            continue;
        }

        HKEY hApp;
        if (RegOpenKeyExW(hParent, name, 0, KEY_READ, &hApp) != ERROR_SUCCESS)
            continue;

        DWORD64 startTime = 0, stopTime = 1;
        DWORD size = sizeof(DWORD64);
        BOOL hasStart = (RegQueryValueExW(hApp, L"LastUsedTimeStart", NULL, NULL,
                                          (BYTE *)&startTime, &size) == ERROR_SUCCESS);
        size = sizeof(DWORD64);
        BOOL hasStop = (RegQueryValueExW(hApp, L"LastUsedTimeStop", NULL, NULL,
                                         (BYTE *)&stopTime, &size) == ERROR_SUCCESS);

        if (hasStart && startTime != 0 && hasStop && stopTime == 0) {
            if (state->appCount < MAX_APPS) {
                ExtractAppName(name, isNonPackaged, state->appNames[state->appCount]);
                state->appCount++;
            }
        }

        RegCloseKey(hApp);
    }
}

static void PollDevices(void) {
    if (g_previewState) return;  /* 预览模式下跳过轮询 */

    /* 重置状态 */
    g_micState.active = FALSE;
    g_micState.appCount = 0;
    g_camState.active = FALSE;
    g_camState.appCount = 0;

    static const WCHAR *basePath =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore";

    HKEY hMic;
    WCHAR micPath[512];
    wsprintfW(micPath, L"%s\\microphone", basePath);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, micPath, 0, KEY_READ, &hMic) == ERROR_SUCCESS) {
        CheckDeviceKeys(hMic, &g_micState, FALSE);
        RegCloseKey(hMic);
    }

    HKEY hCam;
    WCHAR camPath[512];
    wsprintfW(camPath, L"%s\\webcam", basePath);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, camPath, 0, KEY_READ, &hCam) == ERROR_SUCCESS) {
        CheckDeviceKeys(hCam, &g_camState, FALSE);
        RegCloseKey(hCam);
    }

    g_micState.active = (g_micState.appCount > 0);
    g_camState.active = (g_camState.appCount > 0);

    if (g_micState.active) BuildDisplayText(&g_micState);
    if (g_camState.active) BuildDisplayText(&g_camState);

    /* 状态变化检测 */
    BOOL anyNow  = g_micState.active || g_camState.active;
    BOOL anyPrev = g_prevMicActive || g_prevCamActive;

    if (anyNow && !anyPrev) {
        ShowOSDFadeIn();
    } else if (!anyNow && anyPrev) {
        HideOSDFadeOut();
    }
    /* 如果设备组合变化但仍有设备活跃，动画定时器会自动在下一帧更新 */

    g_prevMicActive = g_micState.active;
    g_prevCamActive = g_camState.active;

    /* 更新托盘图标 */
    g_nid.hIcon = anyNow ? g_iconActive : g_iconIdle;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

/* ══════════ GDI+ 辅助 ══════════ */
static void MakeRoundRect(GpPath *path, float x, float y, float w, float h, float r) {
    float d = r * 2;
    GdipAddPathArc(path, x,         y,          d, d, 180, 90);
    GdipAddPathArc(path, x + w - d, y,          d, d, 270, 90);
    GdipAddPathArc(path, x + w - d, y + h - d,  d, d,   0, 90);
    GdipAddPathArc(path, x,         y + h - d,  d, d,  90, 90);
    GdipClosePathFigure(path);
}

/* ══════════ 动画更新 ══════════ */
static void UpdateAnimations(void) {
    /* 麦克风内部填充脉冲 */
    g_micFillPhase += MIC_FILL_SPEED;
    if (g_micFillPhase >= 1.0f)
        g_micFillPhase -= 1.0f;

    /* 四角取景框 */
    g_cornerAngle += CORNER_SPEED;
    if (g_cornerAngle >= (float)(2.0 * M_PI))
        g_cornerAngle -= (float)(2.0 * M_PI);
}

/* ══════════ OSD 绘制 ══════════ */

static void EnsureCache(void) {
    if (g_cacheDC) return;
    HDC hdcScreen = GetDC(NULL);
    g_cacheDC = CreateCompatibleDC(hdcScreen);
    ReleaseDC(NULL, hdcScreen);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth    = g_osdDualW;
    bmi.bmiHeader.biHeight   = -g_osdHeight;
    bmi.bmiHeader.biPlanes   = 1;
    bmi.bmiHeader.biBitCount = 32;
    g_cacheBmp = CreateDIBSection(g_cacheDC, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    g_cacheOldBmp = SelectObject(g_cacheDC, g_cacheBmp);
}

static void FreeCache(void) {
    if (g_cacheDC) {
        SelectObject(g_cacheDC, g_cacheOldBmp);
        DeleteObject(g_cacheBmp);
        DeleteDC(g_cacheDC);
        g_cacheDC = NULL;
        g_cacheBmp = NULL;
        g_cacheOldBmp = NULL;
    }
}

static int CalcOSDWidth(void) {
    BOOL both = g_micState.active && g_camState.active;
    return both ? g_osdDualW : g_osdSingleW;
}

/* 绘制四角取景框 (摄像头) */
static void DrawCornerFrame(GpGraphics *gfx, float cx, float cy, float iconW, float iconH, DWORD animRGB) {
    float phaseT = (sinf(g_cornerAngle) + 1.0f) / 2.0f;  /* 0.0~1.0 */
    float scale  = 0.9f + 0.2f * phaseT;
    BYTE  alpha  = (BYTE)(120 + 135.0f * phaseT);
    DWORD color  = ((DWORD)alpha << 24) | animRGB;

    float gap = CORNER_GAP * scale * g_dpiScale;
    float len = CORNER_LEN * scale * g_dpiScale;

    float x0 = cx - iconW / 2 - gap;
    float y0 = cy - iconH / 2 - gap;
    float x1 = cx + iconW / 2 + gap;
    float y1 = cy + iconH / 2 + gap;

    GpPen *pen = NULL;
    GdipCreatePen1(color, 2.0f * g_dpiScale, UnitPixel, &pen);
    GdipDrawLine(gfx, pen, x0, y0, x0 + len, y0);
    GdipDrawLine(gfx, pen, x0, y0, x0, y0 + len);
    /* 右上 */
    GdipDrawLine(gfx, pen, x1, y0, x1 - len, y0);
    GdipDrawLine(gfx, pen, x1, y0, x1, y0 + len);
    /* 左下 */
    GdipDrawLine(gfx, pen, x0, y1, x0 + len, y1);
    GdipDrawLine(gfx, pen, x0, y1, x0, y1 - len);
    /* 右下 */
    GdipDrawLine(gfx, pen, x1, y1, x1 - len, y1);
    GdipDrawLine(gfx, pen, x1, y1, x1, y1 - len);

    GdipDeletePen(pen);
}

/* 麦克风头部填充脉冲动画 (方案C: 进度条式填充) */
static void DrawMicFill(GpGraphics *gfx, float cx, float cy, DWORD animRGB) {
    float sc = g_dpiScale;
    float iconSize = 36.0f * sc;

    /* 胶囊头部内侧区域（矩形柱，仅顶部微圆角） */
    float barW   = iconSize * 0.33f;
    float capH   = iconSize * 0.50f;           /* 胶囊内部总高度 */
    float capBot = cy + iconSize * 0.12f;       /* 胶囊底部 y (再下移) */
    float barX   = cx - barW / 2 - 1.5f * sc;  /* 微调偏左对齐图标 */

    /* 填充高度脉冲 0..1 */
    float phase = (sinf(g_micFillPhase * 2.0f * (float)M_PI) + 1.0f) / 2.0f;
    float minFill = capH * 0.12f;
    float fillH   = minFill + (capH - minFill) * phase;

    float fillTop = capBot - fillH;

    /* 半透明填充 */
    BYTE  alpha = (BYTE)(90 + 70.0f * phase);   /* 90~160 */
    DWORD color = ((DWORD)alpha << 24) | animRGB;

    /* 矩形柱：底部平直，顶部微圆角 */
    float topR = 2.0f * sc;  /* 仅顶部小圆角 */
    GpPath *fillPath = NULL;
    GdipCreatePath(FillModeAlternate, &fillPath);
    /* 从左下开始顺时针: 左下→左上(圆角)→右上(圆角)→右下→闭合 */
    GdipAddPathLine(fillPath, barX, capBot, barX, fillTop + topR);
    GdipAddPathArc(fillPath, barX, fillTop, topR * 2, topR * 2, 180.0f, 90.0f);
    GdipAddPathLine(fillPath, barX + topR, fillTop, barX + barW - topR, fillTop);
    GdipAddPathArc(fillPath, barX + barW - topR * 2, fillTop, topR * 2, topR * 2, 270.0f, 90.0f);
    GdipAddPathLine(fillPath, barX + barW, fillTop + topR, barX + barW, capBot);
    GdipClosePathFigure(fillPath);

    GpSolidFill *br = NULL;
    GdipCreateSolidFill(color, &br);
    GdipFillPath(gfx, br, fillPath);
    GdipDeleteBrush(br);
    GdipDeletePath(fillPath);
}

/* 手绘麦克风图标 (GDI+ 路径, 支持缩放) */
static void DrawMicIcon(GpGraphics *gfx, float cx, float cy, float s, DWORD color) {
    GpPen *pen = NULL;
    GdipCreatePen1(color, 2.0f * s, UnitPixel, &pen);

    /* 麦克风头部 (填充胶囊) */
    float capW = 6.0f * s, capH = 9.0f * s;
    float capX = cx - capW / 2, capY = cy - 7.0f * s;
    GpPath *cap = NULL;
    GdipCreatePath(FillModeAlternate, &cap);
    GdipAddPathArc(cap, capX, capY, capW, capW, 180.0f, 180.0f);
    GdipAddPathArc(cap, capX, capY + capH - capW, capW, capW, 0.0f, 180.0f);
    GdipClosePathFigure(cap);
    /* 半透明填充 + 描边 */
    DWORD fillColor = (color & 0x00FFFFFFu) | 0x40000000u;
    GpSolidFill *fillBr = NULL;
    GdipCreateSolidFill(fillColor, &fillBr);
    GdipFillPath(gfx, fillBr, cap);
    GdipDeleteBrush(fillBr);
    GdipDrawPath(gfx, pen, cap);
    GdipDeletePath(cap);

    /* 支架弧线 */
    float arcW = 10.0f * s, arcH = 6.0f * s;
    GdipDrawArc(gfx, pen, cx - arcW / 2, cy + 0.5f * s, arcW, arcH, 0.0f, 180.0f);

    /* 立杆 */
    GdipDrawLine(gfx, pen, cx, cy + 0.5f * s + arcH / 2, cx, cy + 7.0f * s);

    /* 底座 */
    GdipDrawLine(gfx, pen, cx - 3.5f * s, cy + 7.0f * s, cx + 3.5f * s, cy + 7.0f * s);

    GdipDeletePen(pen);
}

/* 手绘摄像头图标 (GDI+ 路径, 支持缩放) */
static void DrawCamIcon(GpGraphics *gfx, float cx, float cy, float s, DWORD color) {
    GpPen *pen = NULL;
    GdipCreatePen1(color, 2.0f * s, UnitPixel, &pen);

    /* 主体: 圆角矩形 */
    float bw = 11.0f * s, bh = 8.0f * s, br = 2.0f * s;
    float bx = cx - bw / 2, by = cy - bh / 2;
    GpPath *body = NULL;
    GdipCreatePath(FillModeAlternate, &body);
    MakeRoundRect(body, bx, by, bw, bh, br);
    GdipDrawPath(gfx, pen, body);
    GdipDeletePath(body);

    /* 镜头三角形 */
    float tx0 = bx + bw + 0.5f * s;
    float ty0 = cy - 3.5f * s;
    float tx1 = tx0 + 4.5f * s;
    float ty1 = cy + 3.5f * s;
    GdipDrawLine(gfx, pen, tx0, ty0, tx1, cy);
    GdipDrawLine(gfx, pen, tx1, cy, tx0, ty1);
    GdipDrawLine(gfx, pen, tx0, ty1, tx0, ty0);

    GdipDeletePen(pen);
}

/* 绘制一列设备信息 (CapsBar 风格: 图标在上, 文字在下) */
static void DrawDeviceColumn(GpGraphics *gfx, float colX, float colW,
                             BOOL isMic, const WCHAR *appText,
                             DWORD clrIcon, DWORD clrText, DWORD animRGB) {
    float cx = colX + colW / 2;
    float cy = g_iconCY;

    /* 动画 */
    if (isMic) {
        DrawMicFill(gfx, cx, cy, animRGB);   /* 胶囊内填充脉冲 */
    } else {
        DrawCornerFrame(gfx, cx - 0.5f * g_dpiScale, cy - 2.0f * g_dpiScale, 34.0f * g_dpiScale, 26.0f * g_dpiScale, animRGB);
    }

    /* 设备图标 */
    if (g_fontIcon) {
        /* 使用 Segoe MDL2 Assets 字体图标 */
        /* U+E720 = Microphone, U+E714 = Video */
        const WCHAR micGlyph[] = { 0xE720, 0 };
        const WCHAR camGlyph[] = { 0xE714, 0 };
        const WCHAR *glyph = isMic ? micGlyph : camGlyph;
        GpSolidFill *iconBr = NULL;
        GdipCreateSolidFill(clrIcon, &iconBr);
        float iconSize = 36.0f * g_dpiScale;
        GpRectF rcIcon = { cx - iconSize / 2, cy - iconSize / 2, iconSize, iconSize };
        GdipDrawString(gfx, glyph, 1, g_fontIcon, &rcIcon, g_sfCenter, iconBr);
        GdipDeleteBrush(iconBr);
    } else {
        /* 回退: 手绘矢量图标 */
        if (isMic) {
            DrawMicIcon(gfx, cx, cy, g_dpiScale, clrIcon);
        } else {
            DrawCamIcon(gfx, cx, cy, g_dpiScale, clrIcon);
        }
    }

    /* 应用名称 (居中) */
    GpRectF rcText = { colX + 4.0f * g_dpiScale, g_textY, colW - 8.0f * g_dpiScale, g_textH };
    GpSolidFill *textBr = NULL;
    GdipCreateSolidFill(clrText, &textBr);
    GdipDrawString(gfx, appText, -1, g_fontText, &rcText, g_sfCenter, textBr);
    GdipDeleteBrush(textBr);
}

static void RenderOSD(void) {
    EnsureCache();

    if (!g_micState.active && !g_camState.active) return;

    int osdW = CalcOSDWidth();

    g_darkMode = GetEffectiveDarkMode();
    DWORD clrBG    = g_darkMode ? DARK_BG      : LITE_BG;
    DWORD clrBdr   = g_darkMode ? DARK_BORDER  : LITE_BORDER;
    DWORD clrIcon  = g_darkMode ? DARK_ICON    : LITE_ICON;
    DWORD clrText  = g_darkMode ? DARK_TEXT    : LITE_TEXT;
    DWORD animRGB  = g_darkMode ? DARK_ANIM_RGB : LITE_ANIM_RGB;

    GpGraphics *gfx = NULL;
    GdipCreateFromHDC(g_cacheDC, &gfx);
    GdipGraphicsClear(gfx, 0x00000000);
    GdipSetSmoothingMode(gfx, SmoothingModeAntiAlias);
    GdipSetTextRenderingHint(gfx, TextRenderingHintAntiAlias);

    /* 背景圆角矩形 */
    GpPath *bgPath = NULL;
    GdipCreatePath(FillModeAlternate, &bgPath);
    MakeRoundRect(bgPath, 0.5f, 0.5f, (float)osdW - 1, (float)g_osdHeight - 1, g_osdCorner);

    GpSolidFill *bgBr = NULL;
    GdipCreateSolidFill(clrBG, &bgBr);
    GdipFillPath(gfx, bgBr, bgPath);
    GdipDeleteBrush(bgBr);

    GpPen *borderPen = NULL;
    GdipCreatePen1(clrBdr, 1.0f, UnitPixel, &borderPen);
    GdipDrawPath(gfx, borderPen, bgPath);
    GdipDeletePen(borderPen);
    GdipDeletePath(bgPath);

    /* 绘制设备列 */
    BOOL both = g_micState.active && g_camState.active;
    float colW = both ? (float)g_osdDualW / 2 : (float)g_osdSingleW;

    if (g_micState.active) {
        DrawDeviceColumn(gfx, 0, colW, TRUE, g_micState.displayText, clrIcon, clrText, animRGB);
    }
    if (g_camState.active) {
        float camX = both ? colW : 0;
        DrawDeviceColumn(gfx, camX, colW, FALSE, g_camState.displayText, clrIcon, clrText, animRGB);
    }

    /* 竖线分隔符 (双设备时) */
    if (both) {
        DWORD sepColor = g_darkMode ? 0x26FB923Cu : 0x14000000u;
        GpPen *sepPen = NULL;
        GdipCreatePen1(sepColor, 1.0f, UnitPixel, &sepPen);
        GdipDrawLine(gfx, sepPen, colW, 22.0f * g_dpiScale, colW, (float)g_osdHeight - 22.0f * g_dpiScale);
        GdipDeletePen(sepPen);
    }

    GdipDeleteGraphics(gfx);
}

static void CompositeOSD(void) {
    if (!g_cacheDC) return;
    if (!g_micState.active && !g_camState.active) return;

    int osdW = CalcOSDWidth();

    POINT ptSrc = {0, 0};
    SIZE  szWnd = {osdW, g_osdHeight};
    BLENDFUNCTION blend = {0};
    blend.BlendOp             = AC_SRC_OVER;
    blend.SourceConstantAlpha = g_alpha;
    blend.AlphaFormat         = AC_SRC_ALPHA;

    HMONITOR hMon = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(hMon, &mi);
    RECT rc = mi.rcWork;

    POINT ptDst = {
        rc.right - osdW - g_rightGap,
        rc.bottom - g_osdHeight - g_bottomGap
    };

    HDC hdcScreen = GetDC(NULL);
    UpdateLayeredWindow(g_hwndOSD, hdcScreen, &ptDst, &szWnd,
                        g_cacheDC, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(NULL, hdcScreen);
}

/* ══════════ 显示 / 隐藏 ══════════ */
static void ShowOSDFadeIn(void) {
    g_fadingOut = FALSE;
    g_fadingIn  = TRUE;
    g_alpha     = 0;

    /* 初始化动画相位 */
    g_micFillPhase = 0.0f;
    g_cornerAngle = 0.0f;

    EnsureCache();
    RenderOSD();       /* 先渲染首帧 */
    CompositeOSD();    /* 先 UpdateLayeredWindow (alpha=0, 透明) 避免闪烁 */
    ShowWindow(g_hwndOSD, SW_SHOWNOACTIVATE);
    g_visible = TRUE;
    SetTimer(g_hwndOSD, TIMER_ANIM, ANIM_INTERVAL, NULL);
}

static void HideOSDFadeOut(void) {
    g_fadingIn  = FALSE;
    g_fadingOut = TRUE;
    /* 动画定时器在 alpha 到 0 时会停止自己 */
}

/* ══════════ 托盘图标 ══════════ */
static HICON CreateOnAirTrayIcon(DWORD bgColor, DWORD fgColor) {
    int sz = 32;
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth    = sz;
    bmi.bmiHeader.biHeight   = -sz;
    bmi.bmiHeader.biPlanes   = 1;
    bmi.bmiHeader.biBitCount = 32;
    void *bits = NULL;
    HBITMAP hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HBITMAP oldBmp = SelectObject(hdcMem, hbm);

    GpGraphics *g = NULL;
    GdipCreateFromHDC(hdcMem, &g);
    GdipGraphicsClear(g, 0x00000000);
    GdipSetSmoothingMode(g, SmoothingModeAntiAlias);
    GdipSetTextRenderingHint(g, TextRenderingHintAntiAlias);

    /* 圆角矩形背景 */
    GpPath *p = NULL;
    GdipCreatePath(FillModeAlternate, &p);
    MakeRoundRect(p, 1, 1, (float)(sz - 2), (float)(sz - 2), 7);
    GpSolidFill *bgBr = NULL;
    GdipCreateSolidFill(bgColor, &bgBr);
    GdipFillPath(g, bgBr, p);
    GdipDeleteBrush(bgBr);
    GdipDeletePath(p);

    /* 图标文字 */
    GpFontFamily *ff = NULL;
    GdipCreateFontFamilyFromName(L"Segoe UI", NULL, &ff);
    GpFont *font = NULL;
    GdipCreateFont(ff, 14.0f, 1, UnitPixel, &font); /* Bold */
    GpStringFormat *sf = NULL;
    GdipCreateStringFormat(0, 0, &sf);
    GdipSetStringFormatAlign(sf, StringAlignmentCenter);
    GdipSetStringFormatLineAlign(sf, StringAlignmentCenter);
    GpSolidFill *txtBr = NULL;
    GdipCreateSolidFill(fgColor, &txtBr);
    GpRectF rcTxt = {0, 0, (float)sz, (float)sz};
    GdipDrawString(g, L"ON", 2, font, &rcTxt, sf, txtBr);
    GdipDeleteBrush(txtBr);
    GdipDeleteFont(font);
    GdipDeleteFontFamily(ff);
    GdipDeleteStringFormat(sf);
    GdipDeleteGraphics(g);

    SelectObject(hdcMem, oldBmp);
    BYTE maskBits[128];
    ZeroMemory(maskBits, sizeof(maskBits));
    HBITMAP hbmMask = CreateBitmap(sz, sz, 1, 1, maskBits);
    ICONINFO ii = {0};
    ii.fIcon    = TRUE;
    ii.hbmColor = hbm;
    ii.hbmMask  = hbmMask;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hbm);
    DeleteObject(hbmMask);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return hIcon;
}

static void CreateTray(HWND hwnd) {
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd   = hwnd;
    g_nid.uID    = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon  = g_iconIdle;
    lstrcpyW(g_nid.szTip, g_lang->tip);
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void RemoveTray(void) {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

/* ══════════ 窗口过程 ══════════ */
static LRESULT CALLBACK OSDWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_TIMER:
        if (wp == TIMER_ANIM) {
            /* 淡入淡出 */
            if (g_fadingIn) {
                if (g_alpha < 255 - FADE_STEP_VAL)
                    g_alpha += FADE_STEP_VAL;
                else {
                    g_alpha = 255;
                    g_fadingIn = FALSE;
                }
            }
            if (g_fadingOut) {
                if (g_alpha > FADE_STEP_VAL)
                    g_alpha -= FADE_STEP_VAL;
                else {
                    g_alpha = 0;
                    g_fadingOut = FALSE;
                    ShowWindow(g_hwndOSD, SW_HIDE);
                    g_visible = FALSE;
                    KillTimer(hwnd, TIMER_ANIM);
                    FreeCache();
                    return 0;
                }
            }
            /* 更新动画帧 + 重绘 */
            UpdateAnimations();
            RenderOSD();
            CompositeOSD();
        }
        else if (wp == TIMER_POLL) {
            PollDevices();
        }
        return 0;

    case WM_TRAYICON:
        if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hm = CreatePopupMenu();

            AppendMenuW(hm, MF_STRING | (IsAutoStartEnabled() ? MF_CHECKED : 0),
                        IDM_AUTOSTART, g_lang->menuAutostart);

            HMENU hmTheme = CreatePopupMenu();
            AppendMenuW(hmTheme, MF_STRING | (g_themeMode == 0 ? MF_CHECKED : 0),
                        IDM_THEME_AUTO, g_lang->menuThemeAuto);
            AppendMenuW(hmTheme, MF_STRING | (g_themeMode == 1 ? MF_CHECKED : 0),
                        IDM_THEME_DARK, g_lang->menuThemeDark);
            AppendMenuW(hmTheme, MF_STRING | (g_themeMode == 2 ? MF_CHECKED : 0),
                        IDM_THEME_LIGHT, g_lang->menuThemeLight);
            AppendMenuW(hm, MF_POPUP, (UINT_PTR)hmTheme, g_lang->menuTheme);

            HMENU hmLang = CreatePopupMenu();
            AppendMenuW(hmLang, MF_STRING | (g_langMode == 0 ? MF_CHECKED : 0),
                        IDM_LANG_AUTO, g_lang->menuLangAuto);
            AppendMenuW(hmLang, MF_STRING | (g_langMode == 1 ? MF_CHECKED : 0),
                        IDM_LANG_ZH, g_lang->menuLangZH);
            AppendMenuW(hmLang, MF_STRING | (g_langMode == 2 ? MF_CHECKED : 0),
                        IDM_LANG_EN, g_lang->menuLangEN);
            AppendMenuW(hm, MF_POPUP, (UINT_PTR)hmLang, g_lang->menuLang);

            AppendMenuW(hm, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hm, MF_STRING | (g_previewState ? MF_CHECKED : 0),
                        IDM_PREVIEW, g_lang->menuPreview);
            AppendMenuW(hm, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hm, MF_STRING, IDM_EXIT, g_lang->menuExit);

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hm, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hm);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDM_EXIT:
            DestroyWindow(hwnd);
            break;
        case IDM_AUTOSTART:
            SetAutoStart(!IsAutoStartEnabled());
            break;
        case IDM_THEME_AUTO:  g_themeMode = 0; SaveConfig(); break;
        case IDM_THEME_DARK:  g_themeMode = 1; SaveConfig(); break;
        case IDM_THEME_LIGHT: g_themeMode = 2; SaveConfig(); break;
        case IDM_LANG_AUTO:
            g_langMode = 0; ApplyLanguage();
            lstrcpyW(g_nid.szTip, g_lang->tip);
            Shell_NotifyIconW(NIM_MODIFY, &g_nid);
            SaveConfig();
            break;
        case IDM_LANG_ZH:
            g_langMode = 1; ApplyLanguage();
            lstrcpyW(g_nid.szTip, g_lang->tip);
            Shell_NotifyIconW(NIM_MODIFY, &g_nid);
            SaveConfig();
            break;
        case IDM_LANG_EN:
            g_langMode = 2; ApplyLanguage();
            lstrcpyW(g_nid.szTip, g_lang->tip);
            Shell_NotifyIconW(NIM_MODIFY, &g_nid);
            SaveConfig();
            break;
        case IDM_PREVIEW:
            if (g_previewState) {
                /* 关闭预览: 恢复真实状态 */
                g_previewState = 0;
                g_micState.active = FALSE; g_micState.appCount = 0;
                g_camState.active = FALSE; g_camState.appCount = 0;
                g_prevMicActive = FALSE; g_prevCamActive = FALSE;
                HideOSDFadeOut();
            } else {
                /* 循环预览: 同时显示双设备 */
                g_previewState = 3;
                g_micState.active = TRUE; g_micState.appCount = 1;
                lstrcpyW(g_micState.appNames[0], L"Teams");
                BuildDisplayText(&g_micState);
                g_camState.active = TRUE; g_camState.appCount = 1;
                lstrcpyW(g_camState.appNames[0], L"Zoom");
                BuildDisplayText(&g_camState);
                g_prevMicActive = TRUE; g_prevCamActive = TRUE;
                ShowOSDFadeIn();
            }
            break;
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ANIM);
        KillTimer(hwnd, TIMER_POLL);
        RemoveTray();
        PostQuitMessage(0);
        return 0;

    default:
        if (msg == WM_TASKBARCREATED && WM_TASKBARCREATED != 0) {
            Shell_NotifyIconW(NIM_ADD, &g_nid);
            return 0;
        }
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ══════════ GDI+ 字体初始化 ══════════ */
static void InitFonts(void) {
    GdipCreateFontFamilyFromName(L"Segoe UI", NULL, &g_ffUI);
    GdipCreateFontFamilyFromName(L"Segoe MDL2 Assets", NULL, &g_ffIcon);

    GdipCreateFont(g_ffUI, 14.0f * g_dpiScale, 1, UnitPixel, &g_fontText);
    if (g_ffIcon)
        GdipCreateFont(g_ffIcon, 32.0f * g_dpiScale, 0, UnitPixel, &g_fontIcon);

    GdipCreateStringFormat(0, 0, &g_sfCenter);
    GdipSetStringFormatAlign(g_sfCenter, StringAlignmentCenter);
    GdipSetStringFormatLineAlign(g_sfCenter, StringAlignmentCenter);

}

static void FreeFonts(void) {
    if (g_sfCenter) GdipDeleteStringFormat(g_sfCenter);
    if (g_fontIcon) GdipDeleteFont(g_fontIcon);
    if (g_fontText) GdipDeleteFont(g_fontText);
    if (g_ffIcon)   GdipDeleteFontFamily(g_ffIcon);
    if (g_ffUI)     GdipDeleteFontFamily(g_ffUI);
}

/* ══════════ 入口 ══════════ */
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmdLine, int nShow) {
    (void)hPrev; (void)cmdLine; (void)nShow;
    g_hInst = hInst;

    EnableDPIAwareness();  /* 必须在创建窗口之前调用 */
    ApplyDPIScale();       /* 根据系统 DPI 缩放所有尺寸 */

    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"OnAir_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return 0;
    }

    GdiplusStartupInput gdipInput = {1, NULL, FALSE, FALSE};
    GdiplusStartup(&g_gdipToken, &gdipInput, NULL);

    InitFonts();
    InitConfigPath();
    LoadConfig();
    ApplyLanguage();

    /* 托盘图标 */
    g_iconIdle   = CreateOnAirTrayIcon(0xFF78716Cu, 0xFFFFFFFFu);  /* 灰底白字(空闲) */
    g_iconActive = CreateOnAirTrayIcon(0xFFEA580Cu, 0xFFFFFFFFu);  /* 橙底白字(活跃) */

    WNDCLASSEXW wc = {0};
    wc.cbSize       = sizeof(wc);
    wc.lpfnWndProc  = OSDWndProc;
    wc.hInstance     = hInst;
    wc.hIcon        = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APP),
                                        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = L"OnAirOSD";
    RegisterClassExW(&wc);

    g_hwndOSD = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        L"OnAirOSD", NULL, WS_POPUP,
        0, 0, g_osdDualW, g_osdHeight,
        NULL, NULL, hInst, NULL);

    g_darkMode = GetEffectiveDarkMode();

    WM_TASKBARCREATED = RegisterWindowMessageW(L"TaskbarCreated");
    CreateTray(g_hwndOSD);

    /* 启动轮询定时器 */
    SetTimer(g_hwndOSD, TIMER_POLL, POLL_INTERVAL, NULL);

    /* 立即检测一次 */
    PollDevices();

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    FreeCache();
    FreeFonts();
    if (g_iconIdle)   DestroyIcon(g_iconIdle);
    if (g_iconActive) DestroyIcon(g_iconActive);
    GdiplusShutdown(g_gdipToken);
    CloseHandle(hMutex);
    return 0;
}
