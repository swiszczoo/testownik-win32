#include <screen_welcome.h>

#define CLASS_NAME          L"WelcomeScreenClass"
static ATOM class_atom;

void screen_welcome_register() {
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = screen_welcome_wndproc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = CLASS_NAME;

    class_atom = RegisterClassEx(&wcex);
}

HWND screen_welcome_hwnd(screen_welcome* instance)
{
    return instance->hwnd;
}

void screen_welcome_create(HWND parent, screen_welcome* instance)
{
    instance->hwnd = CreateWindowEx(WS_EX_COMPOSITED, CLASS_NAME, L"",
        WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, parent, NULL, GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(instance->hwnd, GWLP_USERDATA, (LONG_PTR)instance);

    button_modern_create(instance->hwnd, &instance->start_btn, 100, 100, 250, 80);
    button_modern_set_color(&instance->start_btn, RGB(33, 120, 60));
}

LRESULT CALLBACK screen_welcome_wndproc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    screen_welcome* instance = NULL;
    HWND parent_hwnd = hwnd;
    while (parent_hwnd) {
        if (GetClassWord(hwnd, GCW_ATOM) == class_atom) {
            instance = (screen_welcome*)GetWindowLongPtr(parent_hwnd, GWLP_USERDATA);
            break;
        }
        parent_hwnd = GetParent(parent_hwnd);
    }

    switch (msg) {
    case WM_COMMAND:
        Beep(1000, 100);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
