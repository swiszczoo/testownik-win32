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
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        typeface_name
    );
}
