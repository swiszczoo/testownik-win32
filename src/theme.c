#include <theme.h>

#include <Uxtheme.h>
#include <vssym32.h>

#include <stdint.h>

#include <IatHook.h>

// Color tables

static const COLORREF COLOR_TABLE[2][COL_COUNT] = {
    {   // Light theme
        RGB(255, 255, 255),                 // COL_BACKGROUND
        RGB(0, 0, 0),                       // COL_FOREGROUND
        RGB(96, 96, 96),                    // COL_TITLE
        RGB(0, 51, 153),                    // COL_HEADER
        RGB(32, 32, 32),                    // COL_BUTTON_NORMAL
        RGB(196, 196, 196),                 // COL_BUTTON_DISABLED
        RGB(39, 121, 177),                  // COL_BUTTON_PRIMARY
        RGB(168, 168, 168),                 // COL_QUESTION_NUMBER
        RGB(16, 16, 16),                    // COL_QUESTION_TEXT
        RGB(34, 175, 67),                   // COL_BUTTON_CORRECT
        RGB(176, 40, 38),                   // COL_BUTTON_WRONG
        RGB(175, 95, 34),                   // COL_BUTTON_PARTIALLY
        RGB(228, 250, 233),                 // COL_BACKGROUND_CORRECT
        RGB(249, 228, 227),                 // COL_BACKGROUND_WRONG
        RGB(250, 236, 226),                 // COL_BACKGROUND_PARTIALLY
        RGB(240, 240, 240),                 // COL_STATUS_BACKGROUND
    },
    {   // Dark theme
        RGB(32, 32, 32),                    // COL_BACKGROUND
        RGB(240, 240, 240),                 // COL_FOREGROUND
        RGB(168, 168, 168),                 // COL_TITLE
        RGB(140, 177, 255),                 // COL_HEADER
        RGB(80, 80, 80),                    // COL_BUTTON_NORMAL
        RGB(38, 38, 38),                    // COL_BUTTON_DISABLED
        RGB(39, 121, 177),                  // COL_BUTTON_PRIMARY
        RGB(128, 128, 128),                 // COL_QUESTION_NUMBER
        RGB(240, 240, 240),                 // COL_QUESTION_TEXT
        RGB(34, 175, 67),                   // COL_BUTTON_CORRECT
        RGB(176, 40, 38),                   // COL_BUTTON_WRONG
        RGB(175, 95, 34),                   // COL_BUTTON_PARTIALLY
        RGB(32, 51, 34),                    // COL_BACKGROUND_CORRECT
        RGB(51, 32, 32),                    // COL_BACKGROUND_WRONG
        RGB(51, 37, 32),                    // COL_BACKGROUND_PARTIALLY
        RGB(42, 42, 42),                    // COL_STATUS_BACKGROUND
    }
};

// Some nasty undocumented shit right below...

typedef enum
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
} PreferredAppMode;

typedef BOOL(WINAPI* fnShouldAppsUseDarkMode)(); // ordinal 132
typedef BOOL(WINAPI* fnAllowDarkModeForWindow)(HWND hWnd, BOOL allow); // ordinal 133
typedef PreferredAppMode(WINAPI* fnSetPreferredAppMode)(PreferredAppMode appMode); // ordinal 135, in 1903
typedef HRESULT(WINAPI* fnSetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
typedef HRESULT(WINAPI* fnDwmSetWindowAttribute)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
typedef HTHEME(WINAPI* fnOpenNcThemeData)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49


static HWND top_parent = NULL;
static HMODULE dll_ux_theme = NULL;
static fnShouldAppsUseDarkMode ShouldAppsUseDarkMode;
static fnAllowDarkModeForWindow AllowDarkModeForWindow;
static fnSetPreferredAppMode SetPreferredAppMode;
static fnSetWindowTheme SetWindowThemeDynamic;
static fnOpenNcThemeData OpenNcThemeDataDynamic;
static bool dark_mode_supported;
static bool dark_mode_enabled;

static HTHEME MyOpenThemeData(HWND hWnd, LPCWSTR classList)
{
    if (wcscmp(classList, L"ScrollBar") == 0)
    {
        if (GetParent(hWnd) == top_parent) {
            hWnd = NULL;
            classList = L"Explorer::ScrollBar";
        }
    }

    return OpenNcThemeDataDynamic(hWnd, classList);
};

static void theme_fix_scrollbars(void)
{
    HMODULE hComctl = LoadLibraryExW(L"comctl32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hComctl)
    {
        PIMAGE_THUNK_DATA addr = FindDelayLoadThunkInModuleOrd(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
        if (addr)
        {
            DWORD oldProtect;
            if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
            {
                addr->u1.Function = (ULONGLONG)MyOpenThemeData;
                VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
            }
        }
    }
}

void theme_setup_dark_mode(HWND main_wnd)
{
    top_parent = main_wnd;

    // https://gist.github.com/rounk-ctrl/b04e5622e30e0d62956870d5c22b7017
    dll_ux_theme = LoadLibraryEx(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!dll_ux_theme) {
        return;
    }

    ShouldAppsUseDarkMode = (fnShouldAppsUseDarkMode)GetProcAddress(dll_ux_theme, MAKEINTRESOURCEA(132));
    AllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(dll_ux_theme, MAKEINTRESOURCEA(133));
    SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(dll_ux_theme, MAKEINTRESOURCEA(135));
    SetWindowThemeDynamic = (fnSetWindowTheme)GetProcAddress(dll_ux_theme, "SetWindowTheme");
    OpenNcThemeDataDynamic = (fnOpenNcThemeData)GetProcAddress(dll_ux_theme, MAKEINTRESOURCEA(49));

    dark_mode_supported = ShouldAppsUseDarkMode
        && AllowDarkModeForWindow && SetPreferredAppMode
        && SetWindowThemeDynamic && OpenNcThemeDataDynamic;

    if (dark_mode_supported) {
        theme_fix_scrollbars();
        dark_mode_enabled = ShouldAppsUseDarkMode();

        SetPreferredAppMode(AllowDark);
        AllowDarkModeForWindow(main_wnd, true);
        SetWindowThemeDynamic(main_wnd, L"DarkMode_Explorer", NULL);

        if (dark_mode_enabled) {
            HMODULE dll_dwm = LoadLibraryEx(L"dwmapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (dll_dwm) {
                fnDwmSetWindowAttribute DwmSetWindowAttributeDynamic =
                    (fnDwmSetWindowAttribute)GetProcAddress(dll_dwm, "DwmSetWindowAttribute");

                if (DwmSetWindowAttributeDynamic) {
                    BOOL value = true;
                    DwmSetWindowAttributeDynamic(main_wnd, 20, &value, sizeof(value));
                }

                FreeLibrary(dll_dwm);
            }
        }
    }
}

void theme_setup_status_bar(HWND status_bar)
{
    if (!dark_mode_supported) {
        return;
    }

    SetWindowThemeDynamic(status_bar, L"ExplorerStatusBar", NULL);
    AllowDarkModeForWindow(status_bar, TRUE);
    SendMessage(status_bar, WM_THEMECHANGED, 0, 0);
}

void theme_setup_scroll_bar(HWND scroll_bar)
{
    if (!dark_mode_supported) {
        return;
    }

    SetWindowThemeDynamic(scroll_bar, L"Explorer", NULL);
    AllowDarkModeForWindow(scroll_bar, TRUE);
    SendMessage(scroll_bar, WM_THEMECHANGED, 0, 0);
}

void theme_destroy(void)
{
    dark_mode_supported = false;
    dark_mode_enabled = false;

    ShouldAppsUseDarkMode = NULL;
    AllowDarkModeForWindow = NULL;
    SetPreferredAppMode = NULL;
    SetWindowThemeDynamic = NULL;
    OpenNcThemeDataDynamic = NULL;

    FreeLibrary(dll_ux_theme);
    dll_ux_theme = NULL;
}

void theme_set_window_theme(HWND window, LPCWSTR param1, LPCWSTR param2)
{
    if (dark_mode_supported) {
        SetWindowThemeDynamic(window, param1, param2);
    }
}

bool theme_is_dark_theme(void)
{
    return dark_mode_enabled;
}

COLORREF theme_get_color(testownik_color index)
{
    if (index < COL_COUNT && index >= 0) {
        return COLOR_TABLE[dark_mode_enabled ? 1 : 0][index];
    }

    return RGB(0, 0, 0);
}

COLORREF theme_get_performance_color(int percent)
{
    int base = dark_mode_enabled ? 255 : 128;
    int blue = dark_mode_enabled ? 128 : 0;

    int red = base;
    int green = base;

    if (percent < 50) {
        green = blue + percent * (base - blue) / 50;
    }
    if (percent > 50) {
        red = blue + (100 - percent) * (base - blue) / 50;
    }

    return RGB(red, green, blue);
}

COLORREF theme_get_performance_bg_color(int percent)
{
    if (dark_mode_enabled) {
        const int base = 42;
        const int blue = 20;

        int red = base;
        int green = base;

        if (percent < 50) {
            green = blue + percent * (base - blue) / 50;
        }
        if (percent > 50) {
            red = blue + (100 - percent) * (base - blue) / 50;
        }

        return RGB(red, green, blue);
    }

    const int base = 220;
    int red = 255;
    int green = 255;

    if (percent < 50) {
        green = base + (percent) * (255 - base) / 50;
    }

    if (percent > 50) {
        red = base + (100 - percent) * (255 - base) / 50;
    }

    return RGB(red, green, base);
}
