#include <button_modern.h>

#include <CommCtrl.h>

#define BORDER_SIZE 4

static HCURSOR pointer;

HWND button_modern_hwnd(button_modern* instance)
{
    return instance->hwnd;
}

void button_modern_create(HWND parent, button_modern* instance, int x, int y, int width, int height)
{
    instance->hwnd = CreateWindowEx(WS_EX_COMPOSITED, L"BUTTON", L"Start",
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE, x, y, width, height, parent,
        NULL, GetModuleHandle(NULL), NULL);
    instance->org_proc = (WNDPROC)SetWindowLongPtr(
        instance->hwnd, GWLP_WNDPROC, (LONG_PTR)button_modern_wndproc);
    instance->bg_brush = NULL;
    instance->fg_font = CreateFontA(
        36,
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        "Segoe UI"
    );
    instance->hovered = false;
    instance->clicked = false;

    SetWindowLongPtr(instance->hwnd, GWLP_USERDATA, (LONG_PTR)instance);

    button_modern_set_color(instance, RGB(32, 32, 32));
}

void button_modern_destroy(button_modern* instance)
{
    DestroyWindow(instance->hwnd);
    DeleteObject(instance->bg_brush);
    DeleteObject(instance->bg_brush_pressed);
    DeleteObject(instance->bg_pen);
    DeleteObject(instance->fg_font);
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

    instance->bg_brush = CreateSolidBrush(background);

    int pen_r = GetRValue(background);
    int pen_g = GetGValue(background);
    int pen_b = GetBValue(background);

    int pressed_rgb = RGB(pen_r / 2, pen_g / 2, pen_b / 2);
    instance->bg_brush_pressed = CreateSolidBrush(pressed_rgb);

    pen_r += (255 - pen_r) * 2 / 3;
    pen_g += (255 - pen_g) * 2 / 3;
    pen_b += (255 - pen_b) * 2 / 3;

    instance->bg_pen = CreatePen(PS_SOLID, BORDER_SIZE, RGB(pen_r, pen_g, pen_b));
    InvalidateRect(instance->hwnd, NULL, TRUE);
}

LRESULT CALLBACK button_modern_wndproc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    button_modern* instance = (button_modern*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HGDIOBJ prevBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        HGDIOBJ prevPen = SelectObject(hdc, instance->hovered ? instance->bg_pen : GetStockObject(NULL_PEN));
        HGDIOBJ prevFont = SelectObject(hdc, instance->fg_font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));

        if (instance->hovered) {
            Rectangle(hdc, ps.rcPaint.left + BORDER_SIZE / 2, ps.rcPaint.top + BORDER_SIZE / 2,
                ps.rcPaint.right - BORDER_SIZE / 2, ps.rcPaint.bottom - BORDER_SIZE / 2);
        }

        SelectObject(hdc, instance->clicked ? instance->bg_brush_pressed : instance->bg_brush);
        SelectObject(hdc, GetStockObject(NULL_PEN));
        Rectangle(hdc, ps.rcPaint.left + BORDER_SIZE, ps.rcPaint.top + BORDER_SIZE,
            ps.rcPaint.right - BORDER_SIZE, ps.rcPaint.bottom - BORDER_SIZE);

        WCHAR windowText[256];
        GetWindowText(hwnd, windowText, 256);
        DrawText(hdc, windowText, -1, &ps.rcPaint, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
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
    }

    }

    return CallWindowProc(instance->org_proc, hwnd, msg, wParam, lParam);
}
