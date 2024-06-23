#pragma once
#include <Windows.h>

#include <stdbool.h>

typedef enum {
    COL_BACKGROUND,
    COL_FOREGROUND,
    COL_TITLE,
    COL_HEADER,
    COL_BUTTON_NORMAL,
    COL_BUTTON_DISABLED,
    COL_BUTTON_PRIMARY,
    COL_QUESTION_NUMBER,
    COL_QUESTION_TEXT,
    COL_BUTTON_CORRECT,
    COL_BUTTON_WRONG,
    COL_BUTTON_PARTIALLY,
    COL_BACKGROUND_CORRECT,
    COL_BACKGROUND_WRONG,
    COL_BACKGROUND_PARTIALLY,

    COL_COUNT,
} testownik_color;

void theme_setup_dark_mode(HWND main_wnd);
void theme_setup_status_bar(HWND status_bar);
void theme_setup_scroll_bar(HWND scroll_bar);
void theme_destroy(void);
void theme_set_window_theme(HWND window, LPCWSTR param1, LPCWSTR param2);
bool theme_is_dark_theme(void);

COLORREF theme_get_color(testownik_color index);
COLORREF theme_get_performance_color(int percent);
