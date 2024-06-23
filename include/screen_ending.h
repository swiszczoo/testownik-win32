#pragma once
#include <Windows.h>

#include <status_bar.h>

typedef struct {
    HWND hwnd;
    status_bar* status_bar;

    HWND restart_all_btn;
    HWND restart_wrong_btn;
    HWND exit_btn;

    HFONT title_fnt;
    HFONT body_fnt;
    HFONT value_font;

    int stat_total_seconds;
    int stat_correct_answers;
    int stat_total_answers;
    int stat_correct_percent;

    HBRUSH bg_brush;
} screen_ending;

void screen_ending_register(void);
HWND screen_ending_hwnd(screen_ending* instance);
void screen_ending_create(HWND parent, screen_ending* instance, status_bar* status_bar);
void screen_ending_destroy(screen_ending* instance);
void screen_ending_run(screen_ending* instance);

LRESULT CALLBACK screen_ending_wndproc(HWND, UINT, WPARAM, LPARAM);
