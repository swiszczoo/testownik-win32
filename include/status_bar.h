#pragma once
#include "performance_bar.h"

#include <Windows.h>

#include <stdbool.h>

#define STATUS_BAR_STRING_MAX_LEN       64

typedef struct {
    bool question_mode;
    int percent_correct;

    TCHAR question_text[STATUS_BAR_STRING_MAX_LEN];
    TCHAR time_text[STATUS_BAR_STRING_MAX_LEN];
    TCHAR correct_text[STATUS_BAR_STRING_MAX_LEN];
    TCHAR wrong_text[STATUS_BAR_STRING_MAX_LEN];
    TCHAR performance_text[STATUS_BAR_STRING_MAX_LEN];
} status_bar_data;

typedef struct {
    HWND hwnd;

    performance_bar performance;
    HWND question_part;
    HWND time_part;
    HWND correct_part;
    HWND wrong_part;
    HWND performance_part;

    HBRUSH normal_bg_brush;
    HBRUSH question_bg_brush;
    HBRUSH normal_gripper_brush;
    HBRUSH gripper_brush;
    HFONT status_font;

    status_bar_data data;
    bool is_maximized;
    int height;
    COLORREF question_bg_color;
    COLORREF question_text_color;
    int prev_percent;
} status_bar;

void status_bar_register(void);
HWND status_bar_hwnd(status_bar* instance);
void status_bar_create(HWND parent, status_bar* instance);
void status_bar_destroy(status_bar* instance);

void status_bar_resize(status_bar* instance);
void status_bar_update(status_bar* instance, status_bar_data* new_data);

LRESULT CALLBACK status_bar_wndproc(HWND, UINT, WPARAM, LPARAM);
