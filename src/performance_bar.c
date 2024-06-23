#include <performance_bar.h>
#include <theme.h>

#define CLASS_NAME          L"PerformanceBarClass"

static ATOM class_atom;

void performance_bar_register(void) {
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = performance_bar_wndproc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = CLASS_NAME;

    class_atom = RegisterClassEx(&wcex);
}

HWND performance_bar_hwnd(performance_bar* instance)
{
    return instance->hwnd;
}

void performance_bar_create(HWND parent, performance_bar* instance,
    int x, int y, int width, int height)
{
    instance->hwnd = CreateWindowEx(0, CLASS_NAME, L"",
        WS_CHILD | WS_VISIBLE, x, y, width, height, parent, NULL,
        GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(instance->hwnd, GWLP_USERDATA, (LONG_PTR)instance);
    performance_bar_set_value(instance, 50);
}

void performance_bar_destroy(performance_bar* instance)
{
    DestroyWindow(instance->hwnd);

    if (instance->question_bg_brush) {
        DeleteObject(instance->question_bg_brush);
        instance->question_bg_brush = NULL;
    }
}

void performance_bar_set_value(performance_bar* instance, int value)
{
    instance->value = value;

    if (instance->question_bg_brush) {
        DeleteObject(instance->question_bg_brush);
        instance->question_bg_brush = NULL;
    }

    instance->question_bg_brush = CreateSolidBrush(theme_get_performance_bg_color(value));

    InvalidateRect(instance->hwnd, NULL, TRUE);
}

static void performance_bar_paint(performance_bar* instance)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(instance->hwnd, &ps);

    RECT client_rect;
    GetClientRect(instance->hwnd, &client_rect);

    COLORREF color = theme_get_performance_color(instance->value);

    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);

    HGDIOBJ prev_pen = SelectObject(hdc, pen);
    HGDIOBJ prev_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

    Rectangle(hdc,
        client_rect.left, client_rect.top, client_rect.right, client_rect.bottom);

    SelectObject(hdc, GetStockObject(NULL_PEN));
    SelectObject(hdc, brush);

    int right = client_rect.left + (client_rect.right - client_rect.left) * instance->value / 100;
    Rectangle(hdc,
        client_rect.left, client_rect.top, right, client_rect.bottom);

    SelectObject(hdc, prev_pen);
    SelectObject(hdc, prev_brush);

    DeleteObject(pen);
    DeleteObject(brush);

    EndPaint(instance->hwnd, &ps);
}

LRESULT CALLBACK performance_bar_wndproc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    performance_bar* instance = (performance_bar*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {

    case WM_PAINT:
    {
        if (instance) {
            performance_bar_paint(instance);
            return 0;
        }
        break;
    }

    case WM_ERASEBKGND:
    {
        HDC dc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(dc, &rc, instance->question_bg_brush);
        return TRUE;
    }

    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
