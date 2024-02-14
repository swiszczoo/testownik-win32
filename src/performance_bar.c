#include <performance_bar.h>

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
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
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
    instance->value = 50;
}

void performance_bar_destroy(performance_bar* instance)
{
    DestroyWindow(instance->hwnd);
}

void performance_bar_set_value(performance_bar* instance, int value)
{
    instance->value = value;
    InvalidateRect(instance->hwnd, NULL, TRUE);
}

static COLORREF performance_bar_get_color(performance_bar* instance)
{
    int red = 128;
    int green = 128;

    if (instance->value < 50) {
        green = instance->value * 128 / 50;
    }
    if (instance->value > 50) {
        red = (100 - instance->value) * 128 / 50;
    }

    return RGB(red, green, 0);
}

static void performance_bar_paint(performance_bar* instance)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(instance->hwnd, &ps);

    RECT client_rect;
    GetClientRect(instance->hwnd, &client_rect);

    COLORREF color = performance_bar_get_color(instance);

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

    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
