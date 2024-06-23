#pragma once
#include <Windows.h>

#include <button_modern.h>

typedef struct {
    HWND hwnd;
    HWND status_bar;

    button_modern library_btn;
    button_modern start_btn;

    HWND check_rnd_questions;
    HWND check_rnd_answers;
    HWND check_always_multi;
    HWND check_autoselect;

    HFONT title_fnt;
    HFONT header_fnt;
    HFONT body_fnt;

    HBRUSH bg_brush;
} screen_welcome;

void screen_welcome_register(void);
HWND screen_welcome_hwnd(screen_welcome* instance);
void screen_welcome_create(HWND parent, screen_welcome* instance, HWND status_bar);
void screen_welcome_destroy(screen_welcome* instance);
void screen_welcome_run(screen_welcome* instance);

LRESULT CALLBACK screen_welcome_wndproc(HWND, UINT, WPARAM, LPARAM);
