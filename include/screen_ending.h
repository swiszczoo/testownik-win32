#pragma once
#include <Windows.h>

typedef struct {
    HWND hwnd;
    HWND status_bar;

    HFONT title_fnt;
    HFONT body_fnt;
    HFONT value_font;
} screen_ending;

void screen_ending_register(void);
HWND screen_ending_hwnd(screen_ending* instance);
void screen_ending_create(HWND parent, screen_ending* instance, HWND status_bar);
void screen_ending_destroy(screen_ending* instance);
void screen_ending_run(screen_ending* instance);

LRESULT CALLBACK screen_ending_wndproc(HWND, UINT, WPARAM, LPARAM);
