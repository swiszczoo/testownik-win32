#include <screen_question.h>

#include <image_decoder.h>
#include <messages.h>
#include <random.h>
#include <resource.h>
#include <theme.h>
#include <utils.h>

#include <assert.h>
#include <CommCtrl.h>
#include <math.h>
#include <windowsx.h>

#define CLASS_NAME              L"QuestionScreenClass"
#define MAX_LAYOUT_WIDTH        dip(1600)
#define MARGIN                  dip(50)
#define CHECKED_STATE           3
#define RADIOBUTTON_STATE       9
#define CHECKBOX_SIZE           32


typedef enum {
    NORMAL,
    HOVER,
    PRESSED,
    CHECKED_NORMAL,
    CHECKED_HOVER,
    CHECKED_PRESSED,
    ANSWER_OK,
    ANSWER_WRONG,
    ANSWER_EXPECTED
} checkbox_state;

static const LPCWSTR STR_CHECK_ANSWER = L"Sprawd\u017a odpowied\u017a...";
static const LPCWSTR STR_NEXT_QUESTION = L"Nast\u0119pne pytanie...";
static ATOM class_atom;

#define MESSAGES_PER_CATEGORY   10

static const LPCWSTR STR_CORRECT_MSGS[MESSAGES_PER_CATEGORY] = {
    L"Dobrze!",
    L"Poprawna odpowied\u017a!",
    L"\u015awietnie!",
    L"Doskonale!",
    L"Idealnie!",
    L"Brawo!",
    L"Dobra odpowied\u017a!",
    L"Wszystko poprawnie!",
    L"Tak!",
    L"Zgadza si\u0119!",
};

static const LPCWSTR STR_PARTIALLY_MSGS[MESSAGES_PER_CATEGORY] = {
    L"Prawie dobrze!",
    L"Cz\u0119\u015bciowo poprawnie!",
    L"Jeszcze to!",
    L"Zabrak\u0142o tego!",
    L"Ju\u017c prawie dobrze...",
    L"By\u0142o blisko!",
    L"Prawie tak!",
    L"By\u0142o wi\u0119cej poprawnych!",
    L"Ca\u0142kiem, ca\u0142kiem...",
    L"Blisko!",
};

static const LPCWSTR STR_WRONG_MSGS[MESSAGES_PER_CATEGORY] = {
    L"B\u0142\u0119dna odpowied\u017a!",
    L"Niezbyt...",
    L"Nie!",
    L"Niestety, nie!",
    L"Jest inaczej...",
    L"B\u0142\u0105d!",
    L"Nieprawda.",
    L"Spr\u00f3buj jeszcze raz!",
    L"To nie tak!",
    L"Niepoprawna odpowied\u017a!",
};

void screen_question_register() {
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = screen_question_wndproc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = CLASS_NAME;

    class_atom = RegisterClassEx(&wcex);
}

HWND screen_question_hwnd(screen_question* instance)
{
    return instance->hwnd;
}

void screen_question_create(HWND parent, screen_question* instance,
    status_bar* status_bar)
{
    instance->status_bar = status_bar;
    instance->hwnd = CreateWindowEx(WS_EX_COMPOSITED, CLASS_NAME, L"",
        WS_CLIPCHILDREN | WS_CHILD | WS_VISIBLE,
        0, 0, 100, 100, parent, NULL, GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(instance->hwnd, GWLP_USERDATA, (LONG_PTR)instance);

    button_modern_create(instance->hwnd, &instance->check_next_btn,
        STR_CHECK_ANSWER, 0, 0, dip(250), dip(80));
    button_modern_set_color(&instance->check_next_btn, theme_get_color(COL_BUTTON_PRIMARY));
    EnableWindow(button_modern_hwnd(&instance->check_next_btn), FALSE);

    instance->scroll_bar = CreateWindowEx(0, L"SCROLLBAR", NULL,
        WS_CHILD | WS_VISIBLE | SBS_VERT, 0, 0, dip(22), dip(500),
        instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    theme_setup_scroll_bar(instance->scroll_bar);
        
    instance->bmp_checkboxes = (HBITMAP)LoadImage(
        GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CHECKBOXES),
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    instance->dc_checkboxes = NULL;
    instance->dc_checkboxes_orig = NULL;

    instance->question_bmp = NULL;

    instance->question_fnt = create_font(L"Segoe UI", dip(40), false, false);
    instance->answer_fnt = create_font(L"Segoe UI Semibold", dip(30), false, false);
    instance->hint_fnt = create_font(L"Segoe UI", dip(20), false, false);
    instance->bg_brush = CreateSolidBrush(theme_get_color(COL_BACKGROUND));
    instance->timer_ptr = 0;

    instance->correct_bg_brush = CreateSolidBrush(theme_get_color(COL_BACKGROUND_CORRECT));
    instance->wrong_bg_brush = CreateSolidBrush(theme_get_color(COL_BACKGROUND_WRONG));
    instance->partially_bg_brush = CreateSolidBrush(theme_get_color(COL_BACKGROUND_PARTIALLY));
    instance->progress_bar_brush = CreateSolidBrush(theme_get_color(COL_BUTTON_PRIMARY));

    instance->answer_hovered = -1;
    instance->answer_pressed = -1;

    ZeroMemory(&instance->status_data, sizeof(instance->status_data));
    instance->status_data.question_mode = true;
}

void screen_question_destroy(screen_question* instance)
{
    button_modern_destroy(&instance->check_next_btn);

    DestroyWindow(instance->hwnd);

    if (instance->dc_checkboxes) {
        SelectObject(instance->dc_checkboxes, instance->dc_checkboxes_orig);
        DeleteDC(instance->dc_checkboxes);
        instance->dc_checkboxes = NULL;
        instance->dc_checkboxes_orig = NULL;
    }

    if (instance->bmp_checkboxes) {
        DeleteObject(instance->bmp_checkboxes);
        instance->bmp_checkboxes = NULL;
    }

    if (instance->question_fnt) {
        DeleteObject(instance->question_fnt);
        instance->question_fnt = NULL;
    }

    if (instance->answer_fnt) {
        DeleteObject(instance->answer_fnt);
        instance->answer_fnt = NULL;
    }

    if (instance->hint_fnt) {
        DeleteObject(instance->hint_fnt);
        instance->hint_fnt = NULL;
    }

    if (instance->correct_bg_brush) {
        DeleteObject(instance->correct_bg_brush);
        instance->correct_bg_brush = NULL;
    }

    if (instance->wrong_bg_brush) {
        DeleteObject(instance->wrong_bg_brush);
        instance->wrong_bg_brush = NULL;
    }

    if (instance->partially_bg_brush) {
        DeleteObject(instance->partially_bg_brush);
        instance->partially_bg_brush = NULL;
    }

    if (instance->progress_bar_brush) {
        DeleteObject(instance->progress_bar_brush);
        instance->progress_bar_brush = NULL;
    }

    if (instance->bg_brush) {
        DeleteObject(instance->bg_brush);
        instance->bg_brush = NULL;
    }
}

static void screen_question_update_statusbar(screen_question* instance)
{
    testownik_game_state game_state;
    testownik_get_game_state(&game_state);

    instance->question_number = game_state.current_question;
    instance->total_questions = game_state.questions_active_count;

    wsprintf(instance->status_data.question_text, L"Pytanie: %03d / %03d",
        game_state.current_question, game_state.questions_active_count);
    wsprintf(instance->status_data.correct_text, L"Poprawne: %d", game_state.correct_count);
    wsprintf(instance->status_data.wrong_text, L"Niepoprawne: %d", game_state.wrong_count);

    int total_answers = game_state.correct_count + game_state.wrong_count;
    int correct_perc = 50;

    if (total_answers) {
        correct_perc = percent_of(game_state.correct_count, total_answers);
    }

    wsprintf(instance->status_data.performance_text, L" Skuteczno\u015b\u0107: %d%%", correct_perc);
    instance->status_data.percent_correct = correct_perc;

    status_bar_update(instance->status_bar, &instance->status_data);
}

static void screen_question_load_next_question(screen_question* instance)
{
    bool has_question = testownik_move_to_next_question();
    if (!has_question) {
        PostMessage(GetParent(instance->hwnd), TM_END_GAME, 0, 0);
        return;
    }

    testownik_get_question_info(&instance->current_question);

    if (instance->question_dc) {
        SelectObject(instance->question_dc, instance->question_dc_orig);
        DeleteDC(instance->question_dc);
        instance->question_dc = NULL;
        instance->question_dc_orig = NULL;
    }

    if (instance->question_bmp) {
        DeleteBitmap(instance->question_bmp);
        instance->question_bmp = NULL;
    }

    if (instance->current_question.question_image_path[0] != L'\0') {
        instance->question_bmp = image_decoder_file_to_hbitmap(
            instance->current_question.question_image_path);

        if (!instance->question_bmp) {
            MessageBox(instance->hwnd,
                L"To pytanie zawiera obrazek, ale nie uda\u0142o si\u0119 go wczytać.",
                L"Uwaga", MB_ICONWARNING);
        }
    }

    for (int i = 0; i < TESTOWNIK_MAX_ANSWERS_PER_QUESTION; ++i) {
        instance->answer_selected[i] = false;
    }

    instance->correct_answer_message = NULL;
    instance->correct_answer_shown = false;

    SetWindowText(button_modern_hwnd(&instance->check_next_btn), STR_CHECK_ANSWER);
    EnableWindow(button_modern_hwnd(&instance->check_next_btn), false);
    SetFocus(instance->hwnd);
    button_modern_set_color(&instance->check_next_btn, theme_get_color(COL_BUTTON_PRIMARY));
    screen_question_update_statusbar(instance);
    InvalidateRect(instance->hwnd, NULL, true);
}

void screen_question_run(screen_question* instance)
{
    SetFocus(instance->hwnd);
    screen_question_load_next_question(instance);

    instance->timer_seconds = -1;
    instance->timer_ptr = SetTimer(instance->hwnd, 0, 200, NULL);
}

static void screen_question_set_scroll_info(screen_question* instance)
{
    RECT client_rect;
    GetClientRect(instance->hwnd, &client_rect);

    SCROLLINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = instance->total_layout_height + dip(130);
    si.nPage = client_rect.bottom - client_rect.top;
    si.nPos = instance->scroll_position;


    if (si.nPos + si.nPage > si.nMax && si.nPos > 0 && instance->total_layout_height > 0) {
        instance->scroll_position = si.nMax - si.nPage;
        if (instance->scroll_position < 0) {
            instance->scroll_position = 0;
        }
        si.nPos = instance->scroll_position;

        InvalidateRect(instance->hwnd, NULL, TRUE);
    }

    SetScrollInfo(instance->scroll_bar, SB_CTL, &si, TRUE);
    ShowWindow(instance->scroll_bar, si.nMax >= si.nPage ? SW_SHOW : SW_HIDE);
}

static void screen_question_check_answer_next_question(screen_question* instance)
{
    if (!instance->correct_answer_shown) {
        instance->correct_answer_shown = true;

        bool result = testownik_check_answer(instance->answer_selected);
        if (!result) {
            assert(false);
            return;
        }

        testownik_get_question_info(&instance->current_question);
        int message_id = random_next_range(MESSAGES_PER_CATEGORY);

        switch (instance->current_question.result) {
        case CORRECT:
            button_modern_set_color(&instance->check_next_btn, theme_get_color(COL_BUTTON_CORRECT));
            instance->correct_answer_message = STR_CORRECT_MSGS[message_id];
            break;
        case WRONG:
            button_modern_set_color(&instance->check_next_btn, theme_get_color(COL_BUTTON_WRONG));
            instance->correct_answer_message = STR_WRONG_MSGS[message_id];
            break;
        case PARTIALLY_CORRECT:
            button_modern_set_color(&instance->check_next_btn, theme_get_color(COL_BUTTON_PARTIALLY));
            instance->correct_answer_message = STR_PARTIALLY_MSGS[message_id];
            break;
        }

        SetWindowText(button_modern_hwnd(&instance->check_next_btn), STR_NEXT_QUESTION);
        screen_question_update_statusbar(instance);
        InvalidateRect(instance->hwnd, NULL, true);
    }
    else {
        screen_question_load_next_question(instance);
    }
}

static void screen_question_command(screen_question* instance, HWND sender)
{
    if (sender == button_modern_hwnd(&instance->check_next_btn)) {
        screen_question_check_answer_next_question(instance);
    }
}

static void screen_question_resize(screen_question* instance, int width, int height)
{
    RECT client_rect;
    GetClientRect(instance->hwnd, &client_rect);

    int offset_x = MARGIN;
    int viewport_width = MAX_LAYOUT_WIDTH;
    if (client_rect.right - client_rect.left - MARGIN * 2 < MAX_LAYOUT_WIDTH) {
        viewport_width = client_rect.right - client_rect.left - MARGIN * 2;
    }
    else {
        offset_x = (client_rect.right - client_rect.left - MAX_LAYOUT_WIDTH) / 2;
    }

    int y = client_rect.bottom - client_rect.top - dip(130);
    int y2 = instance->layout_end_y;

    int button_y = max(y, y2);

    SetWindowPos(button_modern_hwnd(&instance->check_next_btn), NULL,
        offset_x + dip(100), button_y, viewport_width - dip(200), dip(80), SWP_NOOWNERZORDER);

    int scrollbar_width = dip(22);
    SetWindowPos(instance->scroll_bar, NULL,
        client_rect.right - client_rect.left - scrollbar_width, 0,
        scrollbar_width, client_rect.bottom - client_rect.top, SWP_NOOWNERZORDER);

    screen_question_set_scroll_info(instance);
}

static int screen_question_draw_image(screen_question* instance, HDC hdc,
    int offset_x, int offset_y, int viewport_width)
{
    BITMAP question_bmp;
    GetObject(instance->question_bmp, sizeof(BITMAP), &question_bmp);

    int bitmap_width = question_bmp.bmWidth;
    int bitmap_height = question_bmp.bmHeight;

    int target_width = bitmap_width;
    int target_height = bitmap_height;

    int max_width = viewport_width / 2;

    if (bitmap_width > max_width) {
        target_width = max_width;
        target_height = bitmap_height * max_width / bitmap_width;
    }

    int bitmap_x = offset_x + (viewport_width - target_width) / 2;

    if (!instance->question_dc) {
        instance->question_dc = CreateCompatibleDC(hdc);
        instance->question_dc_orig = SelectObject(instance->question_dc,
            instance->question_bmp);
    }

    BLENDFUNCTION blend;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    AlphaBlend(hdc, bitmap_x, offset_y, target_width, target_height,
        instance->question_dc, 0, 0, bitmap_width, bitmap_height, blend);

    return offset_y + target_height;
}

static int screen_question_draw_answer(screen_question* instance, HDC hdc,
    LPCWSTR text, int offset_x, int offset_y, int viewport_width,
    int checkbox_state, LPRECT out_rect)
{
    RECT answer_rect = { offset_x + dip(32) + 32, offset_y,
        offset_x + viewport_width, offset_y };
    DrawText(hdc, text, -1, &answer_rect, DT_CALCRECT | DT_WORDBREAK);
    DrawText(hdc, text, -1, &answer_rect, DT_NOCLIP | DT_WORDBREAK);

    int checkbox_y = offset_y + (answer_rect.bottom - answer_rect.top - CHECKBOX_SIZE) / 2;
    checkbox_y += dip(2);

    // Draw checkbox
    if (!instance->dc_checkboxes) {
        instance->dc_checkboxes = CreateCompatibleDC(hdc);
        instance->dc_checkboxes_orig = SelectObject(
            instance->dc_checkboxes, instance->bmp_checkboxes);

        if (theme_is_dark_theme()) {
            RGBQUAD dark_colors[16];
            GetDIBColorTable(instance->dc_checkboxes, 0, 16, dark_colors);

            dark_colors[2].rgbRed = 120; dark_colors[2].rgbGreen = 120; dark_colors[2].rgbBlue = 120;
            dark_colors[4].rgbRed = 96; dark_colors[4].rgbGreen = 96; dark_colors[4].rgbBlue = 96;
            dark_colors[7].rgbRed = 64; dark_colors[7].rgbGreen = 64; dark_colors[7].rgbBlue = 64;
            dark_colors[8].rgbRed = 66; dark_colors[8].rgbGreen = 83; dark_colors[8].rgbBlue = 94;
            dark_colors[9].rgbRed = 100; dark_colors[9].rgbGreen = 62; dark_colors[9].rgbBlue = 54;
            dark_colors[10].rgbRed = 60; dark_colors[10].rgbGreen = 84; dark_colors[10].rgbBlue = 106;
            dark_colors[11].rgbRed = 52; dark_colors[11].rgbGreen = 101; dark_colors[11].rgbBlue = 63;
            dark_colors[12].rgbRed = 43; dark_colors[12].rgbGreen = 55; dark_colors[12].rgbBlue = 63;
            dark_colors[13].rgbRed = 40; dark_colors[13].rgbGreen = 40; dark_colors[13].rgbBlue = 40;
            dark_colors[15].rgbRed = 32; dark_colors[15].rgbGreen = 32; dark_colors[15].rgbBlue = 32;

            SetDIBColorTable(instance->dc_checkboxes, 0, 16, dark_colors);
        }
    }

    int bmp_row = checkbox_state / 3;
    int bmp_col = checkbox_state % 3;

    answer_rect.left = offset_x + dip(16);
    InflateRect(&answer_rect, 10, 10);
    *out_rect = answer_rect;

    BitBlt(hdc, offset_x + 16, checkbox_y, 32, 32, instance->dc_checkboxes,
        bmp_col * CHECKBOX_SIZE, bmp_row * CHECKBOX_SIZE, SRCCOPY);
    return answer_rect.bottom + 15;
}

static void screen_question_paint(screen_question* instance)
{
    TCHAR buffer[256];

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(instance->hwnd, &ps);

    if (instance->current_question.question_type == NO_QUESTION) {
        EndPaint(instance->hwnd, &ps);
        return;
    }

    RECT client_rect;
    GetClientRect(instance->hwnd, &client_rect);

    int scroll_y = instance->scroll_position;
    int offset_x = MARGIN;
    int offset_y = MARGIN - scroll_y;
    int viewport_width = MAX_LAYOUT_WIDTH;
    if (client_rect.right - client_rect.left - MARGIN * 2 < MAX_LAYOUT_WIDTH) {
        viewport_width = client_rect.right - client_rect.left - MARGIN * 2;
    }
    else {
        offset_x = (client_rect.right - client_rect.left - MAX_LAYOUT_WIDTH) / 2;
    }

    // Paint code begins here
    HGDIOBJ prev_font = SelectObject(hdc, instance->question_fnt);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, theme_get_color(COL_QUESTION_NUMBER));

    wsprintf(buffer, L"%d. ", instance->current_question.question_number);
    SIZE question_num_size;
    DWORD result = GetTextExtentPoint32(hdc, buffer, wcslen(buffer), &question_num_size);
    RECT question_num_rect = { offset_x, offset_y,
        offset_x + question_num_size.cx, offset_y + question_num_size.cy };
    DrawText(hdc, buffer, -1, &question_num_rect, DT_SINGLELINE | DT_NOCLIP);

    SetTextColor(hdc, theme_get_color(COL_QUESTION_TEXT));
    RECT question_text_rect = { offset_x + question_num_size.cx, offset_y,
        offset_x + viewport_width, offset_y };
    DrawText(hdc, instance->current_question.question_text, -1,
        &question_text_rect, DT_CALCRECT | DT_WORDBREAK);
    DrawText(hdc, instance->current_question.question_text, -1,
        &question_text_rect, DT_WORDBREAK | DT_NOCLIP);

    // Draw question image if it exists
    int current_y = question_text_rect.bottom;
    if (instance->question_bmp) {
        current_y = screen_question_draw_image(instance, hdc,
            offset_x, current_y + dip(20), viewport_width);
    }

    // Draw answers
    int help_text_y = current_y + dip(40);
    current_y += dip(75);

    bool is_pressed = instance->answer_pressed >= 0;
    SelectObject(hdc, instance->answer_fnt);

    for (int i = 0; i < instance->current_question.answer_count; ++i) {
        int state = NORMAL;
        if (instance->current_question.result == QUESTION) {
            // If result is not yet shown

            if (!is_pressed) {
                state = instance->answer_hovered == i ? HOVER : NORMAL;
            }
            else if (instance->answer_pressed == i) {
                state = instance->answer_hovered == i ? PRESSED : HOVER;
            }

            if (instance->answer_selected[i]) {
                state += CHECKED_STATE;
            }
        }
        else {
            // If result is already shown
            static const int states[] = {
                NORMAL, ANSWER_OK, ANSWER_WRONG, NORMAL, ANSWER_EXPECTED, 
            };

            state = states[instance->current_question.answer_state[i]];
        }

        if (instance->current_question.question_type == SINGLE_CHOICE) {
            state += RADIOBUTTON_STATE;
        }

        current_y = screen_question_draw_answer(instance, hdc,
            instance->current_question.answer_text[i], offset_x, current_y,
            viewport_width, state, &instance->answer_rect[i]);
    }

    current_y += dip(15);

    // Draw answer message rect
    HBRUSH brush = instance->correct_bg_brush;
    COLORREF text_color = theme_get_color(COL_TEXT_CORRECT);

    switch (instance->current_question.result) {
    case WRONG:
        brush = instance->wrong_bg_brush;
        text_color = theme_get_color(COL_TEXT_WRONG);
        break;
    case PARTIALLY_CORRECT:
        brush = instance->partially_bg_brush;
        text_color = theme_get_color(COL_TEXT_PARTIALLY);
        break;
    }

    HGDIOBJ prev_brush = SelectObject(hdc, brush);
    HGDIOBJ prev_pen = SelectObject(hdc, GetStockObject(NULL_PEN));

    if (instance->current_question.result != QUESTION) {
        SetTextColor(hdc, text_color);
        SelectObject(hdc, instance->question_fnt);
        
        SIZE text_size;
        DWORD result = GetTextExtentPoint32(hdc,
            instance->correct_answer_message,
            wcslen(instance->correct_answer_message), &text_size);

        text_size.cx += dip(150);
        int pos_x = (client_rect.right - client_rect.left - text_size.cx) / 2;

        int total_height = current_y + dip(250) + scroll_y;
        int additional_offset = 0;

        if (total_height < client_rect.bottom - client_rect.top) {
            additional_offset = (client_rect.bottom - client_rect.top - total_height) / 2;
        }

        RECT text_rect = { pos_x, current_y + additional_offset,
            pos_x + text_size.cx, current_y + dip(80) + additional_offset };
        Rectangle(hdc, pos_x, current_y + additional_offset,
            pos_x + text_size.cx, current_y + dip(80) + additional_offset );
        DrawText(hdc, instance->correct_answer_message, -1, &text_rect,
            DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }

    current_y += dip(120);

    // Draw help text
    SelectObject(hdc, instance->hint_fnt);
    SetTextColor(hdc, theme_get_color(COL_FOREGROUND));
    RECT hint_rect = { offset_x + dip(64), help_text_y,
        offset_x + viewport_width, help_text_y + dip(35) };
    switch (instance->current_question.question_type) {
    case MULTI_CHOICE:
        DrawText(hdc, L"Zaznacz wszystkie poprawne odpowiedzi:", -1,
            &hint_rect, DT_NOCLIP | DT_SINGLELINE);
        break;
    case SINGLE_CHOICE:
        DrawText(hdc, L"Zaznacz poprawn\u0105 odpowied\u017a:", -1,
            &hint_rect, DT_NOCLIP | DT_SINGLELINE);
        break;
    default:
        break;
    }

    // Draw progress bar
    int width = (instance->question_number - 1) * client_rect.right / instance->total_questions;
    SelectObject(hdc, GetStockObject(NULL_PEN));
    SelectObject(hdc, instance->progress_bar_brush);

    Rectangle(hdc, 0, 0, width, 5);

    int total_layout_height = current_y + scroll_y;
    int layout_end_y = current_y;

    if (total_layout_height != instance->total_layout_height
        || layout_end_y != instance->layout_end_y) {

        instance->total_layout_height = total_layout_height;
        instance->layout_end_y = layout_end_y;

        screen_question_resize(instance, client_rect.right - client_rect.left,
            client_rect.bottom - client_rect.top);
    }

    SelectObject(hdc, prev_pen);
    SelectObject(hdc, prev_brush);
    SelectObject(hdc, prev_font);
    EndPaint(instance->hwnd, &ps);
}

static void screen_question_handle_toggle(screen_question* instance, int option)
{
    if (instance->current_question.result != QUESTION) {
        return;
    }

    if (instance->current_question.question_type == MULTI_CHOICE) {
        instance->answer_selected[option] = !instance->answer_selected[option];
    }

    if (instance->current_question.question_type == SINGLE_CHOICE) {
        memset(instance->answer_selected, 0, sizeof(instance->answer_selected));
        instance->answer_selected[option] = true;
    }

    int selected_answers = 0;
    for (int i = 0; i < TESTOWNIK_MAX_ANSWERS_PER_QUESTION; ++i) {
        if (instance->answer_selected[i]) {
            ++selected_answers;
        }
    }

    EnableWindow(button_modern_hwnd(&instance->check_next_btn), selected_answers > 0);
    InvalidateRect(instance->hwnd, NULL, TRUE);

    if (instance->current_question.question_type == SINGLE_CHOICE
        && testownik_get_configuration()->auto_accept) {

        screen_question_check_answer_next_question(instance);
    }
}

static void screen_question_handle_vscroll(screen_question* instance,
    WPARAM wParam, LPARAM lParam)
{
    SCROLLINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS | SIF_PAGE | SIF_TRACKPOS | SIF_RANGE;
    GetScrollInfo(instance->scroll_bar, SB_CTL, &si);

    instance->scroll_position = si.nPos;
    int max_pos = si.nMax - si.nPage;
    static const int INCREMENT = 15;

    switch (LOWORD(wParam)) {
    case SB_TOP:
        instance->scroll_position = 0;
        break;
    case SB_BOTTOM:
        instance->scroll_position = max_pos;
        break;
    case SB_LINEUP:
        instance->scroll_position -= INCREMENT;
        if (instance->scroll_position < 0) {
            instance->scroll_position = 0;
        }
        break;
    case SB_LINEDOWN:
        instance->scroll_position += INCREMENT;
        if (instance->scroll_position > max_pos) {
            instance->scroll_position = max_pos;
        }
        break;
    case SB_PAGEUP:
        instance->scroll_position -= si.nPage;
        if (instance->scroll_position < 0) {
            instance->scroll_position = 0;
        }
        break;
    case SB_PAGEDOWN:
        instance->scroll_position += si.nPage;
        if (instance->scroll_position > max_pos) {
            instance->scroll_position = max_pos;
        }
        break;
    case SB_THUMBPOSITION:
        instance->scroll_position = si.nTrackPos;
        break;
    case SB_THUMBTRACK:
        instance->scroll_position = si.nTrackPos;
        break;
    }

    screen_question_set_scroll_info(instance);
    InvalidateRect(instance->hwnd, NULL, TRUE);
}

LRESULT CALLBACK screen_question_wndproc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    screen_question* instance = (screen_question*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_COMMAND:
    {
        if (instance) {
            screen_question_command(instance, (HWND)lParam);
        }
        return 0;
    }

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        if (instance) {
            screen_question_resize(instance, width, height);
        }
        return 0;
    }

    case WM_PAINT:
    {
        if (instance) {
            screen_question_paint(instance);
            return 0;
        }
    }

    case WM_ERASEBKGND:
    {
        HDC dc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(dc, &rc, instance->bg_brush);
        return TRUE;
    }

    case WM_MOUSEMOVE:
    {
        if (!instance) {
            break;
        }

        TRACKMOUSEEVENT me;
        ZeroMemory(&me, sizeof(me));
        me.cbSize = sizeof(TRACKMOUSEEVENT);
        me.dwFlags = TME_HOVER | TME_LEAVE;
        me.hwndTrack = hwnd;
        me.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&me);

        RECT client_rect;
        GetClientRect(hwnd, &client_rect);

        int hovered = -1;

        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        POINT pt = { x, y };
        int min_x = MARGIN;
        int max_x = client_rect.right - client_rect.left - MARGIN;

        if (client_rect.right - client_rect.left - MARGIN * 2 > MAX_LAYOUT_WIDTH) {
            min_x += (client_rect.right - client_rect.left - MAX_LAYOUT_WIDTH) / 2;
            max_x -= (client_rect.right - client_rect.left - MAX_LAYOUT_WIDTH) / 2;
        }

        if (x >= min_x && x <= max_x) {
            for (int i = 0; i < instance->current_question.answer_count; ++i) {
                if (PtInRect(&instance->answer_rect[i], pt)) {
                    hovered = i;
                    break;
                }
            }
        }

        if (instance->answer_hovered != hovered) {
            instance->answer_hovered = hovered;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
    {
        if (instance->answer_hovered != -1 || instance->answer_pressed != -1) {
            instance->answer_hovered = -1;
            instance->answer_pressed = -1;
            InvalidateRect(hwnd, NULL, TRUE);
        }

        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        TRACKMOUSEEVENT me;
        ZeroMemory(&me, sizeof(me));
        me.cbSize = sizeof(TRACKMOUSEEVENT);
        me.dwFlags = TME_HOVER | TME_LEAVE;
        me.hwndTrack = hwnd;
        me.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&me);

        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        if (instance->answer_pressed != instance->answer_hovered) {
            instance->answer_pressed = instance->answer_hovered;
            InvalidateRect(hwnd, NULL, TRUE);
        }

        return 0;
    }

    case WM_LBUTTONUP:
    {
        if (instance->answer_pressed == instance->answer_hovered
            && instance->answer_pressed >= 0) {

            screen_question_handle_toggle(instance, instance->answer_pressed);
        }

        if (instance->answer_pressed >= 0) {
            instance->answer_pressed = -1;
            InvalidateRect(hwnd, NULL, TRUE);
        }

        return 0;
    }

    case WM_KEYDOWN:
    {
        WORD key_flags = HIWORD(lParam);
        WORD scan_code = LOBYTE(key_flags);

        WCHAR buffer[16];
        BYTE kbd_state[256];
        if (!GetKeyboardState(kbd_state)) {
            return 0;
        }

        int size = ToUnicode(wParam, scan_code, kbd_state, buffer, 16, 0);
        if (size == 1) {
            if (buffer[0] >= '0' && buffer[0] <= '9') {
                int selection = buffer[0] - '1';
                if (buffer[0] == '0') selection = 10;

                if (selection < instance->current_question.answer_count) {
                    screen_question_handle_toggle(instance, selection);
                }
            }
            else {
                for (int i = 0; i < TESTOWNIK_MAX_ANSWERS_PER_QUESTION; ++i) {
                    if (instance->current_question.answer_symbol[i] == buffer[0]) {
                        screen_question_handle_toggle(instance, i);
                        return 0;
                    }
                }
            }

            if ((buffer[0] == L' ' || buffer[0] == L'\r')
                && IsWindowEnabled(button_modern_hwnd(&instance->check_next_btn))) {

                screen_question_check_answer_next_question(instance);
            }
        }

        return 0;
    }

    case WM_TIMER:
    {
        if (!testownik_is_game_in_progress()) {
            KillTimer(hwnd, wParam);
            return 0;
        }

        TCHAR buffer[256];
        int total_seconds = testownik_get_game_seconds_elapsed(false);

        if (instance && total_seconds != instance->timer_seconds) {
            int hours = total_seconds / 3600;
            int minutes = total_seconds / 60 - hours * 60;
            int seconds = total_seconds - minutes * 60 - hours * 3600;

            wsprintf(instance->status_data.time_text,
                L"Czas nauki: %02d:%02d:%02d", hours, minutes, seconds);
            status_bar_update(instance->status_bar, &instance->status_data);

            instance->timer_seconds = total_seconds;
        }
        return 0;
    }

    case WM_VSCROLL:
    {
        if (instance) {
            screen_question_handle_vscroll(instance, wParam, lParam);
            return 0;
        }
        break;
    }

    case WM_MOUSEWHEEL:
    {
        int raw_delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int actual_delta = -raw_delta / 2;

        SCROLLINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_POS | SIF_PAGE | SIF_TRACKPOS | SIF_RANGE;
        GetScrollInfo(instance->scroll_bar, SB_CTL, &si);

        int max_pos = si.nMax - si.nPage;

        instance->scroll_position += actual_delta;
        if (instance->scroll_position > max_pos) {
            instance->scroll_position = max_pos;
        }

        if (instance->scroll_position < 0) {
            instance->scroll_position = 0;
        }

        if (instance) {
            screen_question_set_scroll_info(instance);
            InvalidateRect(instance->hwnd, NULL, TRUE);
        }
        break;
    }

    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
