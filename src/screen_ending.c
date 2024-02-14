#include <screen_ending.h>
#include <utils.h>

#include <CommCtrl.h>

#define CLASS_NAME          L"EndingScreenClass"
#define BASE_LAYOUT_WIDTH   420
#define BASE_LAYOUT_HEIGHT  680

#define VALUE_COLOR         RGB(29, 101, 157)

static ATOM class_atom;

void screen_ending_register(void) {
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = screen_ending_wndproc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = CLASS_NAME;

    class_atom = RegisterClassEx(&wcex);
}

HWND screen_ending_hwnd(screen_ending* instance)
{
    return instance->hwnd;
}

void screen_ending_create(HWND parent, screen_ending* instance, HWND status_bar)
{
    instance->status_bar = status_bar;
    instance->hwnd = CreateWindowEx(WS_EX_COMPOSITED, CLASS_NAME, L"",
        WS_CLIPCHILDREN | WS_CHILD | WS_VISIBLE,
        0, 0, 100, 100, parent, NULL, GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(instance->hwnd, GWLP_USERDATA, (LONG_PTR)instance);

    instance->restart_all_btn = CreateWindowEx(0, L"BUTTON", L"Od nowa",
        BS_COMMANDLINK | WS_CHILD | WS_VISIBLE,
        0, 0, BASE_LAYOUT_WIDTH, 100, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(instance->restart_all_btn, BCM_SETNOTE, 0,
        (LPARAM)L"Rozpocznij now\u0105 gr\u0119 z tymi samymi pytaniami");

    instance->restart_wrong_btn = CreateWindowEx(0, L"BUTTON",
        L"Tylko b\u0142\u0119dne odpowiedzi", BS_COMMANDLINK | WS_CHILD | WS_VISIBLE,
        0, 0, BASE_LAYOUT_WIDTH, 100, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(instance->restart_wrong_btn, BCM_SETNOTE, 0,
        (LPARAM)L"Odpowiedz ponownie tylko na te pytania, na kt\u00f3re "
        "udzielona zosta\u0142a b\u0142\u0119dna odpowied\u017a");

    instance->exit_btn = CreateWindowEx(0, L"BUTTON", L"Koniec",
        BS_COMMANDLINK | WS_CHILD | WS_VISIBLE,
        0, 0, BASE_LAYOUT_WIDTH, 100, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(instance->exit_btn, BCM_SETNOTE, 0, (LPARAM)L"Wyjd\u017a z Testownika");

    instance->title_fnt = CreateFont(
        70,
        0,
        0,
        0,
        FW_ULTRALIGHT,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI Light"
    );
    instance->body_fnt = create_font(L"Segoe UI", 20, false, false);
    instance->value_font = create_font(L"Segoe UI", 45, true, false);
}

void screen_ending_destroy(screen_ending* instance)
{
    DestroyWindow(instance->hwnd);

    if (instance->title_fnt) {
        DeleteObject(instance->title_fnt);
        instance->title_fnt = NULL;
    }

    if (instance->body_fnt) {
        DeleteObject(instance->body_fnt);
        instance->body_fnt = NULL;
    }

    if (instance->value_font) {
        DeleteObject(instance->value_font);
        instance->value_font = NULL;
    }
}

void screen_ending_run(screen_ending* instance)
{
    SendMessage(instance->status_bar, SB_SETTEXT, 0, (LPARAM)L"\tKoniec gry!");
    SendMessage(instance->status_bar, SB_SETTEXT, 1, (LPARAM)L"");
    SendMessage(instance->status_bar, SB_SETTEXT, 2, (LPARAM)L"");
    SendMessage(instance->status_bar, SB_SETTEXT, 3, (LPARAM)L"");
    SendMessage(instance->status_bar, SB_SETTEXT, 4, (LPARAM)L"");
}

static void screen_ending_command(screen_ending* instance, HWND sender)
{

}

static void screen_ending_resize(screen_ending* instance, int width, int height)
{
    int offset_x = (width - BASE_LAYOUT_WIDTH) / 2;
    int offset_y = (height - BASE_LAYOUT_HEIGHT) / 2;

    set_window_pos(instance->restart_all_btn, offset_x, offset_y + 330);
    set_window_pos(instance->restart_wrong_btn, offset_x, offset_y + 430);
    set_window_pos(instance->exit_btn, offset_x, offset_y + 530);
}

static void screen_ending_format_time(LPWSTR out, int total_seconds)
{
    int hours = total_seconds / 3600;
    int minutes = total_seconds / 60 - hours * 60;
    int seconds = total_seconds - minutes * 60;
    if (hours > 0) {
        wsprintf(out, L"%d godz %d min %d s", hours, minutes, seconds);
    }
    else if (minutes > 0) {
        wsprintf(out, L"%d min %d s", minutes, seconds);
    }
    else {
        wsprintf(out, L"%d s", seconds);
    }
}

static COLORREF screen_ending_get_color(int percent)
{
    int red = 128;
    int green = 128;

    if (percent < 50) {
        green = percent * 128 / 50;
    }
    if (percent > 50) {
        red = (100 - percent) * 128 / 50;
    }

    return RGB(red, green, 0);
}

static void screen_ending_paint(screen_ending* instance)
{
    TCHAR buffer[256];

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(instance->hwnd, &ps);

    RECT client_rect;
    GetClientRect(instance->hwnd, &client_rect);

    int offset_x = (client_rect.right - client_rect.left - BASE_LAYOUT_WIDTH) / 2;
    int offset_y = (client_rect.bottom - client_rect.top - BASE_LAYOUT_HEIGHT) / 2;

    HGDIOBJ prev_font = SelectObject(hdc, instance->title_fnt);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(96, 96, 96));

    RECT header_rect = { offset_x, offset_y + 30, BASE_LAYOUT_WIDTH + offset_x, offset_y + 100 };
    DrawText(hdc, L"Koniec gry!", -1, &header_rect, DT_CENTER | DT_SINGLELINE);

    SIZE part1_size, part2_size, part3_size;
    RECT rect1, rect2, rect3;
    int text_offset_x;

    static const LPCWSTR LEARNING_TIME_STR =
        L"Czas, kt\u00f3ry up\u0142yn\u0105\u0142 na nauk\u0119 to  ";

    static const LPCWSTR QUESTION_NUMBER_STR =
        L"Udzielono odpowiedzi na  ";

    static const LPCWSTR QUESTION_NUMBER2_STR =
        L"  pyta\u0144";

    static const LPCWSTR CORRECT_ANSWERS_STR =
        L"Poprawno\u015b\u0107 udzielanych odpowiedzi to  ";

    // Draw first line
    SelectObject(hdc, instance->body_fnt);
    GetTextExtentPoint32(hdc, LEARNING_TIME_STR, wcslen(LEARNING_TIME_STR), &part1_size);
    SelectObject(hdc, instance->value_font);
    int seconds = 121;
    screen_ending_format_time(buffer, seconds);
    GetTextExtentPoint32(hdc, buffer, wcslen(buffer), &part2_size);
    text_offset_x = (BASE_LAYOUT_WIDTH - part1_size.cx - part2_size.cx) / 2;
    SelectObject(hdc, instance->body_fnt);
    SetTextColor(hdc, RGB(0, 0, 0));
    rect1.left = offset_x + text_offset_x;
    rect1.top = offset_y + 120;
    rect1.right = rect1.left + part1_size.cx;
    rect1.bottom = offset_y + 170;
    DrawText(hdc, LEARNING_TIME_STR, wcslen(LEARNING_TIME_STR), &rect1,
        DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
    SelectObject(hdc, instance->value_font);
    SetTextColor(hdc, VALUE_COLOR);
    rect2.left = rect1.right;
    rect2.top = rect1.top;
    rect2.right = rect2.left + part2_size.cx;
    rect2.bottom = rect1.bottom - 15;
    DrawText(hdc, buffer, wcslen(buffer), &rect2,
        DT_SINGLELINE | DT_VCENTER) | DT_NOCLIP;

    // Draw second line
    SelectObject(hdc, instance->body_fnt);
    GetTextExtentPoint32(hdc, QUESTION_NUMBER_STR, wcslen(QUESTION_NUMBER_STR), &part1_size);
    SelectObject(hdc, instance->value_font);
    int total_questions = 50;
    wsprintf(buffer, L"%d", total_questions);
    GetTextExtentPoint32(hdc, buffer, wcslen(buffer), &part2_size);
    SelectObject(hdc, instance->body_fnt);
    GetTextExtentPoint32(hdc, QUESTION_NUMBER2_STR, wcslen(QUESTION_NUMBER2_STR), &part3_size);
    text_offset_x = (BASE_LAYOUT_WIDTH - part1_size.cx - part2_size.cx - part3_size.cx) / 2;
    SelectObject(hdc, instance->body_fnt);
    SetTextColor(hdc, RGB(0, 0, 0));
    rect1.left = offset_x + text_offset_x;
    rect1.top = offset_y + 170;
    rect1.right = rect1.left + part1_size.cx;
    rect1.bottom = offset_y + 220;
    DrawText(hdc, QUESTION_NUMBER_STR, wcslen(QUESTION_NUMBER_STR), &rect1,
        DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
    SelectObject(hdc, instance->value_font);
    SetTextColor(hdc, VALUE_COLOR);
    rect2.left = rect1.right;
    rect2.top = rect1.top;
    rect2.right = rect2.left + part2_size.cx;
    rect2.bottom = rect1.bottom - 15;
    DrawText(hdc, buffer, wcslen(buffer), &rect2,
        DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
    SelectObject(hdc, instance->body_fnt);
    SetTextColor(hdc, RGB(0, 0, 0));
    rect3.left = rect2.right;
    rect3.top = rect2.top;
    rect3.right = rect3.left + part3_size.cx;
    rect3.bottom = rect1.bottom;
    DrawText(hdc, QUESTION_NUMBER2_STR, wcslen(QUESTION_NUMBER2_STR), &rect3,
        DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);

    // Draw third line
    SelectObject(hdc, instance->body_fnt);
    GetTextExtentPoint32(hdc, CORRECT_ANSWERS_STR, wcslen(CORRECT_ANSWERS_STR), &part1_size);
    SelectObject(hdc, instance->value_font);
    int correct_percent = 100;
    wsprintf(buffer, L"%d%%", correct_percent);
    GetTextExtentPoint32(hdc, buffer, wcslen(buffer), &part2_size);
    text_offset_x = (BASE_LAYOUT_WIDTH - part1_size.cx - part2_size.cx) / 2;
    SelectObject(hdc, instance->body_fnt);
    SetTextColor(hdc, RGB(0, 0, 0));
    rect1.left = offset_x + text_offset_x;
    rect1.top = offset_y + 220;
    rect1.right = rect1.left + part1_size.cx;
    rect1.bottom = offset_y + 270;
    DrawText(hdc, CORRECT_ANSWERS_STR, wcslen(CORRECT_ANSWERS_STR), &rect1,
        DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
    SelectObject(hdc, instance->value_font);
    SetTextColor(hdc, screen_ending_get_color(correct_percent));
    rect2.left = rect1.right;
    rect2.top = rect1.top;
    rect2.right = rect2.left + part2_size.cx;
    rect2.bottom = rect1.bottom - 15;
    DrawText(hdc, buffer, wcslen(buffer), &rect2,
        DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);

    // Draw command link header
    SelectObject(hdc, instance->body_fnt);
    SetTextColor(hdc, RGB(0, 0, 0));
    RECT command_link_rect = { offset_x, offset_y + 300,
        offset_x + BASE_LAYOUT_WIDTH, offset_y + 320 };
    DrawText(hdc, L"Co teraz?", -1, &command_link_rect,
        DT_SINGLELINE | DT_CENTER | DT_NOCLIP);
    
    SelectObject(hdc, prev_font);
    EndPaint(instance->hwnd, &ps);
}

LRESULT CALLBACK screen_ending_wndproc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    screen_ending* instance = (screen_ending*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_COMMAND:
    {
        if (instance) {
            screen_ending_command(instance, (HWND)lParam);
        }
        return 0;
    }
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        if (instance) {
            screen_ending_resize(instance, width, height);
        }
        return 0;
    }
    case WM_PAINT:
    {
        if (instance) {
            screen_ending_paint(instance);
            return 0;
        }
        break;
    }
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    {
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
