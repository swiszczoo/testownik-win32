#include <button_modern.h>
#include <theme.h>
#include <utils.h>

#include <CommCtrl.h>

#define BORDER_SIZE 4

static HCURSOR pointer;

HWND button_modern_hwnd(button_modern* instance)
{
    return instance->hwnd;
}

void button_modern_create(HWND parent, button_modern* instance, LPCWSTR text,
    int x, int y, int width, int height)
{
    instance->hwnd = CreateWindowEx(WS_EX_COMPOSITED, L"BUTTON", text,
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE, x, y, width, height, parent,
        NULL, GetModuleHandle(NULL), NULL);
    instance->org_proc = (WNDPROC)SetWindowLongPtr(
        instance->hwnd, GWLP_WNDPROC, (LONG_PTR)button_modern_wndproc);
    instance->bg_brush = NULL;
    instance->bg_brush_pressed = NULL;
    instance->bg_brush_disabled = CreateSolidBrush(theme_get_color(COL_BUTTON_DISABLED));
    instance->bg_nopen = CreatePen(PS_SOLID, BORDER_SIZE, theme_get_color(COL_BACKGROUND));
    instance->fg_font = create_font(L"Segoe UI", dip(36), true, false);
    instance->hovered = false;
    instance->clicked = false;

    SetWindowLongPtr(instance->hwnd, GWLP_USERDATA, (LONG_PTR)instance);

    WORD class_style = GetClassLongPtr(instance->hwnd, GCL_STYLE);
    class_style &= ~CS_DBLCLKS;
    SetClassLongPtr(instance->hwnd, GCL_STYLE, class_style);

    button_modern_set_color(instance, theme_get_color(COL_BUTTON_NORMAL));
}

void button_modern_destroy(button_modern* instance)
{
    DestroyWindow(instance->hwnd);

    if (instance->bg_brush) {
        DeleteObject(instance->bg_brush);
        instance->bg_brush = NULL;
    }
    if (instance->bg_brush_pressed) {
        DeleteObject(instance->bg_brush_pressed);
        instance->bg_brush_pressed = NULL;
    }
    if (instance->bg_brush_disabled) {
        DeleteObject(instance->bg_brush_disabled);
        instance->bg_brush_disabled = NULL;
    }
    if (instance->bg_pen) {
        DeleteObject(instance->bg_pen);
        instance->bg_pen = NULL;
    }
    if (instance->bg_pen_pressed) {
        DeleteObject(instance->bg_pen_pressed);
        instance->bg_pen_pressed = NULL;
    }
    if (instance->bg_nopen) {
        DeleteObject(instance->bg_nopen);
        instance->bg_nopen = NULL;
    }
    if (instance->fg_font) {
        DeleteObject(instance->fg_font);
        instance->fg_font = NULL;
    }
}

void button_modern_set_color(button_modern* instance, COLORREF background)
{
    instance->bg_color = background;

    if (instance->bg_brush) {
        DeleteObject(instance->bg_brush);
    }
    if (instance->bg_brush_pressed) {
        DeleteObject(instance->bg_brush_pressed);
    }
    if (instance->bg_pen) {
        DeleteObject(instance->bg_pen);
    }
    if (instance->bg_pen_pressed) {
        DeleteObject(instance->bg_pen_pressed);
    }

    instance->bg_brush = CreateSolidBrush(background);

    int pen_r = GetRValue(background);
    int pen_g = GetGValue(background);
    int pen_b = GetBValue(background);

    COLORREF bg_color = theme_get_color(COL_BACKGROUND);

    int bg_r = GetRValue(bg_color);
    int bg_g = GetGValue(bg_color);
    int bg_b = GetBValue(bg_color);

    COLORREF pressed_rgb = RGB(pen_r / 2, pen_g / 2, pen_b / 2);
    instance->bg_brush_pressed = CreateSolidBrush(pressed_rgb);

    pen_r += (bg_r - pen_r) * 2 / 3;
    pen_g += (bg_g - pen_g) * 2 / 3;
    pen_b += (bg_b - pen_b) * 2 / 3;

    instance->bg_pen = CreatePen(PS_SOLID, BORDER_SIZE, RGB(pen_r, pen_g, pen_b));

    if (theme_is_dark_theme()) {
        pen_r = GetRValue(pressed_rgb);
        pen_g = GetGValue(pressed_rgb);
        pen_b = GetBValue(pressed_rgb);

        pen_r += (bg_r - pen_r) * 2 / 3;
        pen_g += (bg_g - pen_g) * 2 / 3;
        pen_b += (bg_b - pen_b) * 2 / 3;
    }

    instance->bg_pen_pressed = CreatePen(PS_SOLID, BORDER_SIZE, RGB(pen_r, pen_g, pen_b));

    InvalidateRect(instance->hwnd, NULL, TRUE);
}

LRESULT CALLBACK button_modern_wndproc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    button_modern* instance = (button_modern*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_PAINT:
    {
        BOOL enabled = IsWindowEnabled(hwnd);
        if (!enabled) {
            instance->hovered = instance->clicked = false;
        }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HGDIOBJ prevBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        HGDIOBJ prevPen = SelectObject(hdc,
            instance->clicked ? instance->bg_pen_pressed :
            (instance->hovered ? instance->bg_pen : instance->bg_nopen));
        HGDIOBJ prevFont = SelectObject(hdc, instance->fg_font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));

        Rectangle(hdc, clientRect.left + BORDER_SIZE / 2, clientRect.top + BORDER_SIZE / 2,
            clientRect.right - BORDER_SIZE / 2 + 1, clientRect.bottom - BORDER_SIZE / 2 + 1);

        if (enabled) {
            SelectObject(hdc, instance->clicked ? instance->bg_brush_pressed : instance->bg_brush);
        }
        else {
            SelectObject(hdc, instance->bg_brush_disabled);
        }

        SelectObject(hdc, GetStockObject(NULL_PEN));
        Rectangle(hdc, clientRect.left + BORDER_SIZE, clientRect.top + BORDER_SIZE,
            clientRect.right - BORDER_SIZE + 1, clientRect.bottom - BORDER_SIZE + 1);

        WCHAR windowText[256];
        GetWindowText(hwnd, windowText, 256);

        DrawText(hdc, windowText, -1, &clientRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        SelectObject(hdc, prevBrush);
        SelectObject(hdc, prevPen);
        SelectObject(hdc, prevFont);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SETCURSOR:
    {
        if (!pointer) {
            pointer = LoadCursor(NULL, IDC_HAND);
        }

        SetCursor(pointer);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT me;
        ZeroMemory(&me, sizeof(me));
        me.cbSize = sizeof(TRACKMOUSEEVENT);
        me.dwFlags = TME_HOVER | TME_LEAVE;
        me.hwndTrack = hwnd;
        me.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&me);

        instance->hovered = true;
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    case WM_LBUTTONDOWN:
    {
        instance->clicked = true;
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    case WM_LBUTTONUP:
    {
        instance->clicked = false;
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    case WM_MOUSELEAVE:
    {
        instance->hovered = false;
        instance->clicked = false;
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    case WM_KEYDOWN:
    {
        SetFocus(GetParent(hwnd));
        return SendMessage(GetParent(hwnd), msg, wParam, lParam);
    }

    case WM_MOUSEACTIVATE:
    {
        return MA_NOACTIVATE;
    }

    }

    return CallWindowProc(instance->org_proc, hwnd, msg, wParam, lParam);
}
