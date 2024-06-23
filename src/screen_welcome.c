#include <screen_welcome.h>

#include <messages.h>
#include <testownik.h>
#include <theme.h>
#include <utils.h>

#include <ShlObj.h>
#include <Shlwapi.h>

#include <windowsx.h>

#define CLASS_NAME          L"WelcomeScreenClass"
#define BASE_LAYOUT_WIDTH   dip(650)
#define BASE_LAYOUT_HEIGHT  dip(660)

#define ABOUT_TEXT L"Ta implementacja Testownika zosta\u0142a napisana w j\u0119zyku C. " \
                   L"Bezpo\u015brednio u\u017cywa interfejs\u00f3w systemu Windows, aby " \
                   L"maksymalnie zmniejszy\u0107 rozmiar pliku binarnego.\r\n" \
                   L"Licencja: MIT\r\n" \
                   L"Repo: https://github.com/swiszczoo/testownik-win32"

static ATOM class_atom;

void screen_welcome_register(void) {
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = screen_welcome_wndproc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = CLASS_NAME;

    class_atom = RegisterClassEx(&wcex);
}

HWND screen_welcome_hwnd(screen_welcome* instance)
{
    return instance->hwnd;
}

void screen_welcome_create(HWND parent, screen_welcome* instance, status_bar* status_bar)
{
    instance->status_bar = status_bar;
    instance->hwnd = CreateWindowEx(WS_EX_COMPOSITED, CLASS_NAME, L"",
        WS_CLIPCHILDREN | WS_CHILD | WS_VISIBLE,
        0, 0, 100, 100, parent, NULL, GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(instance->hwnd, GWLP_USERDATA, (LONG_PTR)instance);

    button_modern_create(instance->hwnd, &instance->library_btn, L"Wyb\u00f3r bazy...",
        0, 0, dip(250), dip(80));
    button_modern_set_color(&instance->library_btn, theme_get_color(COL_BUTTON_NORMAL));

    button_modern_create(instance->hwnd, &instance->start_btn, L"Rozpocznij",
        0, 0, dip(250), dip(80));
    button_modern_set_color(&instance->start_btn, RGB(33, 120, 60));

    EnableWindow(button_modern_hwnd(&instance->start_btn), FALSE);

    instance->check_rnd_questions = CreateWindow(L"BUTTON",
        L"Losowa kolejno\u015b\u0107 pyta\u0144",
        BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        0, 0, dip(630), dip(25), instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    instance->check_rnd_answers = CreateWindow(L"BUTTON",
        L"Losowa kolejno\u015b\u0107 odpowiedzi",
        BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        0, 0, dip(630), dip(25), instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    instance->check_always_multi = CreateWindow(L"BUTTON",
        L"Zawsze pokazuj pytania wielokrotnego wyboru",
        BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        0, 0, dip(630), dip(25), instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    instance->check_autoselect = CreateWindow(L"BUTTON",
        L"Automatycznie zatwierdzaj odpowied\u017a przy pytaniach jednokrotnego wyboru",
        BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        0, 0, dip(630), dip(25), instance->hwnd, NULL, GetModuleHandle(NULL), NULL);

    SendMessage(instance->check_rnd_questions, BM_SETCHECK, BST_CHECKED, (LPARAM)NULL);
    SendMessage(instance->check_rnd_answers, BM_SETCHECK, BST_CHECKED, (LPARAM)NULL);

    instance->title_fnt = CreateFont(
        dip(70),
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
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI Light"
    );
    instance->header_fnt = create_font(L"Segoe UI", dip(28), false, false);
    instance->body_fnt = create_font(L"Segoe UI", dip(20), false, false);

    instance->bg_brush = CreateSolidBrush(theme_get_color(COL_BACKGROUND));

    SendMessage(instance->check_rnd_questions, WM_SETFONT, (WPARAM)instance->body_fnt, TRUE);
    SendMessage(instance->check_rnd_answers, WM_SETFONT, (WPARAM)instance->body_fnt, TRUE);
    SendMessage(instance->check_always_multi, WM_SETFONT, (WPARAM)instance->body_fnt, TRUE);
    SendMessage(instance->check_autoselect, WM_SETFONT, (WPARAM)instance->body_fnt, TRUE);

    if (theme_is_dark_theme()) {
        theme_set_window_theme(instance->check_rnd_questions, L"", L"");
        theme_set_window_theme(instance->check_rnd_answers, L"", L"");
        theme_set_window_theme(instance->check_always_multi, L"", L"");
        theme_set_window_theme(instance->check_autoselect, L"", L"");
    }
}

void screen_welcome_destroy(screen_welcome* instance)
{
    button_modern_destroy(&instance->library_btn);
    button_modern_destroy(&instance->start_btn);

    DestroyWindow(instance->hwnd);

    if (instance->title_fnt) {
        DeleteObject(instance->title_fnt);
        instance->title_fnt = NULL;
    }

    if (instance->header_fnt) {
        DeleteObject(instance->header_fnt);
        instance->header_fnt = NULL;
    }

    if (instance->body_fnt) {
        DeleteObject(instance->body_fnt);
        instance->body_fnt = NULL;
    }

    if (instance->bg_brush) {
        DeleteObject(instance->bg_brush);
        instance->bg_brush = NULL;
    }
}

static int WINAPI screen_welcome_browse_for_db_proc(
    HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    if (msg == BFFM_INITIALIZED) {
        TCHAR start_path[MAX_PATH + 16];
        testownik_get_db_path(start_path, MAX_PATH + 16);

        while (!PathIsDirectory(start_path)) {
            PathRemoveFileSpec(start_path);
        }

        SendMessage(hwnd, BFFM_SETSELECTION, 1, (LPARAM)start_path);
        return 1;
    }

    return 0;
}

static bool screen_welcome_browse_for_db(screen_welcome* instance)
{
    TCHAR previous_path[MAX_PATH + 16];
    testownik_get_db_path(previous_path, MAX_PATH + 16);

    while (true) {
        TCHAR selected_path[MAX_PATH + 16];

        BROWSEINFO bi;
        ZeroMemory(&bi, sizeof(BROWSEINFO));

        bi.hwndOwner = instance->hwnd;
        bi.pidlRoot = NULL;
        bi.pszDisplayName = selected_path;
        bi.lpszTitle = L"Wybierz folder z baz\u0105 pyta\u0144:";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;
        bi.lpfn = screen_welcome_browse_for_db_proc;
        LPITEMIDLIST result = SHBrowseForFolder(&bi);

        if (result != NULL) {
            SHGetPathFromIDList(result, selected_path);
            testownik_set_db_path(selected_path);
            if (!testownik_try_load_database()) {
                MessageBox(instance->hwnd, L"Nie uda\u0142o si\u0119 wczyta\u0107 "
                    L"bazy pyta\u0144 z wybranego folderu. Spr\u00f3buj ponownie.", NULL, MB_ICONWARNING);

                continue;
            }
            else {
                InvalidateRect(instance->hwnd, NULL, TRUE);
                return true;
            }
        }

        testownik_set_db_path(previous_path);
        testownik_try_load_database();

        return false;
    }
}

static void screen_welcome_start_game(screen_welcome* instance)
{
    testownik_config game_config;
    game_config.shuffle_questions = Button_GetCheck(instance->check_rnd_questions) == BST_CHECKED;
    game_config.shuffle_answers = Button_GetCheck(instance->check_rnd_answers) == BST_CHECKED;
    game_config.always_multiselect = Button_GetCheck(instance->check_always_multi) == BST_CHECKED;
    game_config.auto_accept = Button_GetCheck(instance->check_autoselect) == BST_CHECKED;

    testownik_start_game(&game_config);
    PostMessage(GetParent(instance->hwnd), TM_START_GAME, 0, 0);
}

static void screen_welcome_command(screen_welcome* instance, HWND sender)
{
    if (sender == button_modern_hwnd(&instance->library_btn)) {
        screen_welcome_browse_for_db(instance);
    }
    else if (sender == button_modern_hwnd(&instance->start_btn)) {
        screen_welcome_start_game(instance);
    }
}

static void screen_welcome_resize(screen_welcome* instance, int width, int height)
{
    int offset_x = (width - BASE_LAYOUT_WIDTH) / 2;
    int offset_y = (height - BASE_LAYOUT_HEIGHT) / 2;

    set_window_pos(button_modern_hwnd(&instance->library_btn),
        dip(65) + offset_x, dip(540) + offset_y);
    set_window_pos(button_modern_hwnd(&instance->start_btn),
        dip(335) + offset_x, dip(540) + offset_y);
    set_window_pos(instance->check_rnd_questions, dip(10) + offset_x, dip(235) + offset_y);
    set_window_pos(instance->check_rnd_answers, dip(10) + offset_x, dip(260) + offset_y);
    set_window_pos(instance->check_always_multi, dip(10) + offset_x, dip(285) + offset_y);
    set_window_pos(instance->check_autoselect, dip(10) + offset_x, dip(310) + offset_y);
}

static void screen_welcome_paint(screen_welcome* instance)
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
    SetTextColor(hdc, theme_get_color(COL_TITLE));

    RECT header_rect = { offset_x, offset_y + 30,
        BASE_LAYOUT_WIDTH + offset_x, offset_y + dip(100) };
    DrawText(hdc, L"Testownik dla Win32 v" VERSION_STRING, -1, &header_rect, DT_CENTER | DT_SINGLELINE);

    SelectObject(hdc, instance->header_fnt);
    SetTextColor(hdc, theme_get_color(COL_HEADER));
    RECT db_header_rect = { offset_x, offset_y + dip(120),
        BASE_LAYOUT_WIDTH + offset_x, offset_y + dip(150) };
    DrawText(hdc, L"Baza pyta\u0144", -1, &db_header_rect, DT_SINGLELINE);

    RECT conf_header_rect = { offset_x, offset_y + dip(200),
        BASE_LAYOUT_WIDTH + offset_x, offset_y + dip(230) };
    DrawText(hdc, L"Konfiguracja programu", -1, &conf_header_rect, DT_SINGLELINE);

    RECT about_header_rect = { offset_x, offset_y + dip(340),
        BASE_LAYOUT_WIDTH + offset_x, offset_y + dip(370) };
    DrawText(hdc, L"O Testowniku", -1, &about_header_rect, DT_SINGLELINE);

    SelectObject(hdc, instance->body_fnt);
    SetTextColor(hdc, theme_get_color(COL_FOREGROUND));

    RECT db_body_rect = { offset_x + dip(10), offset_y + dip(155),
        dip(BASE_LAYOUT_WIDTH - 10) + offset_x, offset_y + dip(175) };
    TCHAR path[MAX_PATH + 16];
    testownik_get_db_path(path, MAX_PATH + 16);
    DrawText(hdc, path, -1, &db_body_rect, DT_SINGLELINE | DT_PATH_ELLIPSIS);

    RECT db_count_rect = { offset_x + dip(10), offset_y + dip(175),
        dip(BASE_LAYOUT_WIDTH - 10) + offset_x, offset_y + dip(195) };
    wsprintf(buffer,
        L"Znaleziono %d %s w bazie",
        (int)testownik_get_question_count(),
        plural(testownik_get_question_count(), L"pytanie", L"pytania", L"pyta\u0144"));
    DrawText(hdc, buffer, -1, &db_count_rect, DT_SINGLELINE);

    RECT about_body_rect = { offset_x + dip(10), offset_y + dip(375),
        BASE_LAYOUT_WIDTH - dip(10) + offset_x, offset_y + dip(500) };
    DrawText(hdc, ABOUT_TEXT, -1, &about_body_rect, DT_WORDBREAK);

    SelectObject(hdc, prev_font);
    EndPaint(instance->hwnd, &ps);
}

void screen_welcome_run(screen_welcome* instance)
{
    status_bar_data data;
    ZeroMemory(&data, sizeof(data));
    data.question_mode = FALSE;
    wcscpy(data.question_text, L"Testownik \u2014 konfiguracja");

    status_bar_update(instance->status_bar, &data);

    bool ok = testownik_try_load_database();
    if (!ok) {
        if (!screen_welcome_browse_for_db(instance)) {
            PostQuitMessage(0);
        }
    }

    EnableWindow(button_modern_hwnd(&instance->start_btn), TRUE);
    InvalidateRect(instance->hwnd, NULL, TRUE);
}

LRESULT CALLBACK screen_welcome_wndproc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    screen_welcome* instance = (screen_welcome*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_COMMAND:
    {
        if (instance) {
            screen_welcome_command(instance, (HWND)lParam);
        }
        return 0;
    }
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        if (instance) {
            screen_welcome_resize(instance, width, height);
        }
        return 0;
    }
    case WM_PAINT:
    {
        if (instance) {
            screen_welcome_paint(instance);
            return 0;
        }
        break;
    }
    case WM_ERASEBKGND:
    {
        HDC dc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(dc, &rc, instance->bg_brush);
        return TRUE;
    }
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    {
        SetBkMode((HDC)wParam, TRANSPARENT);
        SetTextColor((HDC)wParam, theme_get_color(COL_FOREGROUND));
        return (LRESULT)instance->bg_brush;
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
