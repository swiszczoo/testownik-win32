#pragma once
#include <Windows.h>

#include <stdbool.h>

inline void set_window_pos(HWND hwnd, int x, int y)
{
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE);
}

inline HFONT create_font(LPCWSTR typeface_name, int height, bool bold, bool italic)
{
    return CreateFont(
        height,
        0,
        0,
        0,
        bold ? FW_BOLD : FW_NORMAL,
        italic ? TRUE : FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        typeface_name
    );
}

inline int percent_of(int a, int b)
{
    // https://stackoverflow.com/questions/20788793/c-how-to-calculate-a-percentageperthousands-without-floating-point-precision
    return (100 * a + b / 2) / b;
}

inline LPCTSTR plural(int n, LPCTSTR one, LPCTSTR few, LPCTSTR many)
{
    if (n == 1) return one;
    if (n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 > 20)) return few;
    return many;
}

extern int dip(int input);
