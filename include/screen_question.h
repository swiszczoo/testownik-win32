#pragma once
#include <Windows.h>

#include <button_modern.h>
#include <testownik.h>

typedef struct {
    HWND hwnd;
    HWND status_bar;

    button_modern check_next_btn;

    HBITMAP bmp_checkboxes;
    HDC dc_checkboxes;
    HBITMAP dc_checkboxes_orig;
    testownik_question_info current_question;

    HFONT question_fnt;
    HFONT answer_fnt;
    HFONT hint_fnt;

    RECT answer_rect[TESTOWNIK_MAX_ANSWERS_PER_QUESTION];
    bool answer_selected[TESTOWNIK_MAX_ANSWERS_PER_QUESTION];
    int answer_hovered;
    int answer_pressed;
} screen_question;

void screen_question_register();
HWND screen_question_hwnd(screen_question* instance);
void screen_question_create(HWND parent, screen_question* instance, HWND status_bar);
void screen_question_destroy(screen_question* instance);
void screen_question_run(screen_question* instance);

LRESULT CALLBACK screen_question_wndproc(HWND, UINT, WPARAM, LPARAM);
