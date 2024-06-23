#pragma once
#include <Windows.h>

typedef struct {
    HWND hwnd;
    int value;

    HBRUSH question_bg_brush;
} performance_bar;

void performance_bar_register(void);
HWND performance_bar_hwnd(performance_bar* instance);
void performance_bar_create(HWND parent, performance_bar* instance,
    int x, int y, int width, int height);
void performance_bar_destroy(performance_bar* instance);
void performance_bar_set_value(performance_bar* instance, int value);

LRESULT CALLBACK performance_bar_wndproc(HWND, UINT, WPARAM, LPARAM);
