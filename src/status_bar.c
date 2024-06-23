#include <status_bar.h>
#include <theme.h>
#include <utils.h>

#include <windowsx.h>

#define CLASS_NAME          L"TestownikStatusBarClass"
#define STATUS_BAR_HEIGHT   30

static ATOM class_atom;

void status_bar_register(void) {
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = status_bar_wndproc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = CLASS_NAME;

    class_atom = RegisterClassEx(&wcex);
}

HWND status_bar_hwnd(status_bar* instance)
{
    return instance->hwnd;
}

void status_bar_create(HWND parent, status_bar* instance)
{
    instance->height = dip(STATUS_BAR_HEIGHT);
    instance->hwnd = CreateWindowEx(WS_EX_COMPOSITED, CLASS_NAME, L"",
        WS_CHILD | WS_VISIBLE, 0, instance->height, 0, 0, parent, NULL,
        GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(instance->hwnd, GWLP_USERDATA, (LONG_PTR)instance);
    ZeroMemory(&instance->data, sizeof(instance->data));

    performance_bar_create(instance->hwnd, &instance->performance, 0, 0, 0, 0);

    instance->question_part = CreateWindow(L"STATIC",
        L"", SS_CENTER | SS_ENDELLIPSIS | WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);

    instance->time_part = CreateWindow(L"STATIC",
        L"", SS_CENTER | SS_ENDELLIPSIS | WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);

    instance->correct_part = CreateWindow(L"STATIC",
        L"", SS_CENTER | SS_ENDELLIPSIS | WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);

    instance->wrong_part = CreateWindow(L"STATIC",
        L"", SS_CENTER | SS_ENDELLIPSIS | WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);

    instance->performance_part = CreateWindow(L"STATIC",
        L"", SS_CENTER | SS_ENDELLIPSIS | WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);

    instance->normal_bg_brush = CreateSolidBrush(theme_get_color(COL_STATUS_BACKGROUND));
    instance->status_font = create_font(L"Tahoma", dip(20), TRUE, FALSE);
    instance->question_bg_color = 0;
    instance->prev_percent = -1;
    instance->normal_gripper_brush = CreateSolidBrush(theme_get_color(COL_FOREGROUND));

    SendMessage(instance->question_part, WM_SETFONT, (WPARAM)instance->status_font, TRUE);
    SendMessage(instance->time_part, WM_SETFONT, (WPARAM)instance->status_font, TRUE);
    SendMessage(instance->correct_part, WM_SETFONT, (WPARAM)instance->status_font, TRUE);
    SendMessage(instance->wrong_part, WM_SETFONT, (WPARAM)instance->status_font, TRUE);
    SendMessage(instance->performance_part, WM_SETFONT, (WPARAM)instance->status_font, TRUE);

    status_bar_resize(instance);
    status_bar_update(instance, &instance->data);
}

void status_bar_destroy(status_bar* instance)
{
    DestroyWindow(instance->hwnd);
    performance_bar_destroy(&instance->performance);

    if (instance->normal_bg_brush) {
        DeleteObject(instance->normal_bg_brush);
        instance->normal_bg_brush = NULL;
    }

    if (instance->question_bg_brush) {
        DeleteObject(instance->question_bg_brush);
        instance->question_bg_brush = NULL;
    }

    if (instance->normal_gripper_brush) {
        DeleteObject(instance->normal_gripper_brush);
        instance->normal_gripper_brush = NULL;
    }

    if (instance->gripper_brush) {
        DeleteObject(instance->gripper_brush);
        instance->gripper_brush = NULL;
    }

    if (instance->status_font) {
        DeleteObject(instance->status_font);
        instance->status_font = NULL;
    }
}

void status_bar_resize(status_bar* instance)
{
    RECT parent_rect;
    HWND parent = GetParent(instance->hwnd);

    if (!parent) {
        return;
    }

    GetClientRect(parent, &parent_rect);

    MoveWindow(instance->hwnd, 0, parent_rect.bottom - instance->height,
        parent_rect.right, instance->height, TRUE);

    WINDOWPLACEMENT placement;
    ZeroMemory(&placement, sizeof(placement));
    GetWindowPlacement(parent, &placement);

    instance->is_maximized = placement.showCmd == SW_MAXIMIZE;

    int y_offset = dip(5);
    int text_height = dip(20);

    MoveWindow(instance->question_part, dip(8), y_offset,
        parent_rect.right - dip(978), text_height, TRUE);
    MoveWindow(instance->time_part, parent_rect.right - dip(970), y_offset,
        dip(245), text_height, TRUE);
    MoveWindow(instance->correct_part, parent_rect.right - dip(725), y_offset,
        dip(175), text_height, TRUE);
    MoveWindow(instance->wrong_part, parent_rect.right - dip(550), y_offset,
        dip(200), text_height, TRUE);
    MoveWindow(instance->performance_part, parent_rect.right - dip(350), y_offset,
        dip(200), text_height, TRUE);

    int text_width = dip(150);
    SetWindowPos(performance_bar_hwnd(&instance->performance), NULL,
        parent_rect.right - dip(150), y_offset, dip(300) - text_width - dip(30), dip(20), SWP_NOOWNERZORDER);
}

void status_bar_update(status_bar* instance, status_bar_data* new_data)
{
    memcpy(&instance->data, new_data, sizeof(status_bar_data));
    static LPCWSTR empty = L"";

    SetWindowText(instance->question_part, instance->data.question_text);
    SetWindowText(instance->time_part,
        instance->data.question_mode ? instance->data.time_text : empty);
    SetWindowText(instance->correct_part,
        instance->data.question_mode ? instance->data.correct_text : empty);
    SetWindowText(instance->wrong_part,
        instance->data.question_mode ? instance->data.wrong_text : empty);
    SetWindowText(instance->performance_part,
        instance->data.question_mode ? instance->data.performance_text : empty);
    ShowWindow(performance_bar_hwnd(&instance->performance),
        instance->data.question_mode ? SW_SHOW : SW_HIDE);

    LONG_PTR question_style = GetWindowLongPtr(instance->question_part, GWL_STYLE);
    question_style &= ~(SS_CENTER | SS_LEFT);
    question_style |= new_data->question_mode ? SS_CENTER : SS_LEFT;
    SetWindowLongPtr(instance->question_part, GWL_STYLE, question_style);

    if (new_data->question_mode) {
        COLORREF bg_color = theme_get_performance_bg_color(new_data->percent_correct);
        if (bg_color != instance->question_bg_color) {
            instance->question_bg_color = bg_color;

            if (instance->question_bg_brush) {
                DeleteObject(instance->question_bg_brush);
                instance->question_bg_brush = NULL;
            }

            if (instance->gripper_brush) {
                DeleteObject(instance->gripper_brush);
                instance->gripper_brush = NULL;
            }

            instance->question_bg_brush = CreateSolidBrush(bg_color);
            instance->gripper_brush = CreateSolidBrush(theme_get_performance_color(new_data->percent_correct));
        }

        instance->question_text_color = theme_get_performance_color(new_data->percent_correct);

        if (new_data->percent_correct != instance->prev_percent) {
            instance->prev_percent = new_data->percent_correct;
            performance_bar_set_value(&instance->performance, new_data->percent_correct);
        }
    }

    InvalidateRect(instance->hwnd, NULL, TRUE);
}

static void status_bar_paint(status_bar* instance)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(instance->hwnd, &ps);

    RECT client_rect;
    GetClientRect(instance->hwnd, &client_rect);

    // Draw a gripper if we're not maximized
    if (!instance->is_maximized) {
        int rect_width = dip(3);
        int rect_spacing = dip(5);

        int pos_y = client_rect.bottom;
        for (int y = 2; y >= 0; --y) {
            pos_y -= rect_spacing;
            int pos_x = client_rect.right;
            for (int x = 0; x <= y; ++x) {
                pos_x -= rect_spacing;

                RECT rect;
                rect.left = pos_x;
                rect.top = pos_y;
                rect.right = rect.left + rect_width;
                rect.bottom = rect.top + rect_width;
                FillRect(hdc, &rect,
                    instance->data.question_mode ? instance->gripper_brush : instance->normal_gripper_brush);
            }
        }
    }

    EndPaint(instance->hwnd, &ps);
}

LRESULT CALLBACK status_bar_wndproc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    status_bar* instance = (status_bar*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_ERASEBKGND:
    {
        HDC dc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(dc, &rc,
            instance->data.question_mode ? instance->question_bg_brush : instance->normal_bg_brush);
        return TRUE;
    }
    case WM_CTLCOLORSTATIC:
    {
        SetBkColor((HDC)wParam,
            instance->data.question_mode ? instance->question_bg_color : theme_get_color(COL_STATUS_BACKGROUND));
        SetTextColor((HDC)wParam,
            instance->data.question_mode ? instance->question_text_color : theme_get_color(COL_FOREGROUND));
        return (LRESULT)(instance->data.question_mode ? instance->question_bg_brush : instance->normal_bg_brush);
    }
    case WM_PAINT:
    {
        if (instance) {
            status_bar_paint(instance);
            return 0;
        }
        break;
    }
    case WM_NCHITTEST: {
        int x_pos = GET_X_LPARAM(lParam);
        int y_pos = GET_Y_LPARAM(lParam);

        POINT pnt = { x_pos, y_pos };
        ScreenToClient(hwnd, &pnt);

        RECT client_rect;
        GetClientRect(hwnd, &client_rect);

        if (!instance->is_maximized && pnt.x >= client_rect.right - dip(30)) {
            return HTBOTTOMRIGHT;
        }
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
