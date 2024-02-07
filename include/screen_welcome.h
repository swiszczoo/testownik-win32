#pragma once
#include <Windows.h>

#include <button_modern.h>

typedef struct {
    HWND hwnd;
    button_modern start_btn;
} screen_welcome;

void screen_welcome_register();
HWND screen_welcome_hwnd(screen_welcome* instance);
void screen_welcome_create(HWND parent, screen_welcome* instance);

LRESULT CALLBACK screen_welcome_wndproc(HWND, UINT, WPARAM, LPARAM);
