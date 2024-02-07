#pragma once
#include <Windows.h>

#include <stdbool.h>

typedef struct {
    HWND hwnd;
    COLORREF bg_color;
    HBRUSH bg_brush;
    HBRUSH bg_brush_pressed;
    HPEN bg_pen;
    HFONT fg_font;
    WNDPROC org_proc;

    bool hovered;
    bool clicked;
} button_modern;

HWND button_modern_hwnd(button_modern* instance);
void button_modern_create(HWND parent, button_modern* instance, int x, int y, int width, int height);
void button_modern_destroy(button_modern* instance);
void button_modern_set_color(button_modern* instance, COLORREF background);

LRESULT CALLBACK button_modern_wndproc(HWND, UINT, WPARAM, LPARAM);

