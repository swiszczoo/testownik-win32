#pragma once
#include <Windows.h>

#include <button_modern.h>
#include <performance_bar.h>
#include <testownik.h>

typedef struct {
    HWND hwnd;
    HWND status_bar;

    button_modern check_next_btn;
    performance_bar* performance_bar;
    HWND scroll_bar;

    HBITMAP bmp_checkboxes;
    HDC dc_checkboxes;
    HBITMAP dc_checkboxes_orig;
    testownik_question_info current_question;
    int question_number;
    int total_questions;

    HBITMAP question_bmp;
    HDC question_dc;
    HBITMAP question_dc_orig;

    HFONT question_fnt;
    HFONT answer_fnt;
    HFONT hint_fnt;
    UINT_PTR timer_ptr;

    HBRUSH bg_brush;
    HBRUSH correct_bg_brush;
    HBRUSH wrong_bg_brush;
    HBRUSH partially_bg_brush;
    HBRUSH progress_bar_brush;

    int timer_seconds;
    int scroll_position;
    int total_layout_height;
    int layout_end_y;

    RECT answer_rect[TESTOWNIK_MAX_ANSWERS_PER_QUESTION];
    bool answer_selected[TESTOWNIK_MAX_ANSWERS_PER_QUESTION];
    int answer_hovered;
    int answer_pressed;

    bool correct_answer_shown;
    LPCWSTR correct_answer_message;
} screen_question;

void screen_question_register();
HWND screen_question_hwnd(screen_question* instance);
void screen_question_create(HWND parent, screen_question* instance, HWND status_bar,
    performance_bar* perf_bar);
void screen_question_destroy(screen_question* instance);
void screen_question_run(screen_question* instance);

LRESULT CALLBACK screen_question_wndproc(HWND, UINT, WPARAM, LPARAM);
