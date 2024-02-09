#include <screen_welcome.h>

#include <messages.h>
#include <testownik.h>
#include <utils.h>

#include <ShlObj.h>
#include <Shlwapi.h>

#include <windowsx.h>

#define CLASS_NAME          L"WelcomeScreenClass"
#define BASE_LAYOUT_WIDTH   650
#define BASE_LAYOUT_HEIGHT  620

#define ABOUT_TEXT L"Ta implementacja Testownika zosta\u0142a napisana w j\u0119zyku C. " \
                    "Bezpo\u015brednio u\u017cywa interfejs\u00f3w systemu Windows, aby " \
                    "maksymalnie zmniejszy\u0107 rozmiar pliku binarnego.\r\n" \
                    "Licencja: MIT\r\n" \
                    "Repo: https://github.com/swiszczoo/testownik-win32"

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

void screen_welcome_create(HWND parent, screen_welcome* instance, HWND status_bar)
{
    instance->status_bar = status_bar;
    instance->hwnd = CreateWindowEx(WS_EX_COMPOSITED, CLASS_NAME, L"",
        WS_CLIPCHILDREN | WS_CHILD | WS_VISIBLE,
        0, 0, 100, 100, parent, NULL, GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(instance->hwnd, GWLP_USERDATA, (LONG_PTR)instance);

    button_modern_create(instance->hwnd, &instance->library_btn, L"Wyb\u00f3r bazy...", 0, 0, 250, 80);
    button_modern_set_color(&instance->library_btn, RGB(32, 32, 32));

    button_modern_create(instance->hwnd, &instance->start_btn, L"Rozpocznij", 0, 0, 250, 80);
    button_modern_set_color(&instance->start_btn, RGB(33, 120, 60));

    EnableWindow(button_modern_hwnd(&instance->start_btn), FALSE);

    instance->check_rnd_questions = CreateWindow(L"BUTTON",
        L"Losowa kolejno\u015b\u0107 pyta\u0144", BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        0, 0, 630, 25, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    instance->check_rnd_answers = CreateWindow(L"BUTTON",
        L"Losowa kolejno\u015b\u0107 odpowiedzi", BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        0, 0, 630, 25, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    instance->check_always_multi = CreateWindow(L"BUTTON",
        L"Zawsze pokazuj pytania wielokrotnego wyboru", BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        0, 0, 630, 25, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    instance->check_autoselect = CreateWindow(L"BUTTON",
        L"Automatycznie zatwierdzaj odpowied\u017a przy pytaniach jednokrotnego wyboru",
        BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        0, 0, 630, 25, instance->hwnd, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(instance->check_rnd_questions, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(instance->check_rnd_answers, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(instance->check_always_multi, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(instance->check_autoselect, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    SendMessage(instance->check_rnd_questions, BM_SETCHECK, BST_CHECKED, (LPARAM)NULL);
    SendMessage(instance->check_rnd_answers, BM_SETCHECK, BST_CHECKED, (LPARAM)NULL);

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
    instance->header_fnt = create_font(L"Segoe UI", 28, false, false);
    instance->body_fnt = create_font(L"Segoe UI", 20, false, false);
}

void screen_welcome_destroy(screen_welcome* instance)
{
    button_modern_destroy(&instance->library_btn);
    button_modern_destroy(&instance->start_btn);

    DestroyWindow(instance->hwnd);

    if (instance->title_fnt) {
        DeleteObject(instance->title_fnt);
    }

    if (instance->header_fnt) {
        DeleteObject(instance->header_fnt);
    }

    if (instance->body_fnt) {
        DeleteObject(instance->body_fnt);
    }
}

static int WINAPI screen_welcome_browse_for_db_proc(
    HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    if (msg == BFFM_INITIALIZED) {
        TCHAR start_path[MAX_PATH + 16];
        testownik_get_db_path(start_path, MAX_PATH + 16);

        while (!PathIsDirectory(start_path)) {
            PathCchRemoveFileSpec(start_path, MAX_PATH + 16);
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
        bi.lpszTitle = L"Wybierz folder z baz\u0105 pyta\u0144";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;
        bi.lpfn = screen_welcome_browse_for_db_proc;
        LPITEMIDLIST result = SHBrowseForFolder(&bi);

        if (result != NULL) {
            SHGetPathFromIDList(result, selected_path);
            testownik_set_db_path(selected_path);
            if (!testownik_try_load_database()) {
                MessageBox(instance->hwnd, L"Nie uda\u0142o si\u0119 wczyta\u0107 "
                    "bazy pyta\u0144 z wybranego folderu. Spr\u00f3buj ponownie.", NULL, MB_ICONWARNING);

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

    set_window_pos(button_modern_hwnd(&instance->library_btn), 50 + offset_x, 490 + offset_y);
    set_window_pos(button_modern_hwnd(&instance->start_btn), 325 + offset_x, 490 + offset_y);
    set_window_pos(instance->check_rnd_questions, 10 + offset_x, 235 + offset_y);
    set_window_pos(instance->check_rnd_answers, 10 + offset_x, 260 + offset_y);
    set_window_pos(instance->check_always_multi, 10 + offset_x, 285 + offset_y);
    set_window_pos(instance->check_autoselect, 10 + offset_x, 310 + offset_y);
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
    SetTextColor(hdc, RGB(96, 96, 96));

    RECT header_rect = { offset_x, offset_y + 30, 650 + offset_x, offset_y + 100 };
    DrawText(hdc, L"Testownik (win32)", -1, &header_rect, DT_CENTER | DT_SINGLELINE);

    SelectObject(hdc, instance->header_fnt);
    SetTextColor(hdc, RGB(0, 51, 153));
    RECT db_header_rect = { offset_x, offset_y + 120, 650 + offset_x, offset_y + 150 };
    DrawText(hdc, L"Baza pyta\u0144", -1, &db_header_rect, DT_SINGLELINE);

    RECT conf_header_rect = { offset_x, offset_y + 200, 650 + offset_x, offset_y + 230 };
    DrawText(hdc, L"Konfiguracja programu", -1, &conf_header_rect, DT_SINGLELINE);

    RECT about_header_rect = { offset_x, offset_y + 340, 650 + offset_x, offset_y + 370 };
    DrawText(hdc, L"O Testowniku", -1, &about_header_rect, DT_SINGLELINE);

    SelectObject(hdc, instance->body_fnt);
    SetTextColor(hdc, RGB(0, 0, 0));

    RECT db_body_rect = { offset_x + 10, offset_y + 155, 650 + offset_x, offset_y + 175 };
    TCHAR path[MAX_PATH + 16];
    testownik_get_db_path(path, MAX_PATH + 16);
    DrawText(hdc, path, -1, &db_body_rect, DT_SINGLELINE | DT_PATH_ELLIPSIS);

    RECT db_count_rect = { offset_x + 10, offset_y + 175, 650 + offset_x, offset_y + 195 };
    wsprintf(buffer, L"Liczba pyta\u0144: %d", (int)testownik_get_question_count());
    DrawText(hdc, buffer, -1, &db_count_rect, DT_SINGLELINE);

    RECT about_body_rect = { offset_x + 10, offset_y + 375, 650 + offset_x, offset_y + 455 };
    DrawText(hdc, ABOUT_TEXT, -1, &about_body_rect, DT_WORDBREAK);

    SelectObject(hdc, prev_font);
    EndPaint(instance->hwnd, &ps);
}

void screen_welcome_run(screen_welcome* instance)
{
    SendMessage(instance->status_bar, SB_SETTEXT, 0, (LPARAM)L"Testownik win32 - konfiguracja");

    bool ok = testownik_try_load_database();
    if (!ok) {
        if (!screen_welcome_browse_for_db(instance)) {
            PostQuitMessage(0);
        }
    }

    EnableWindow(button_modern_hwnd(&instance->start_btn), TRUE);
    InvalidateRect(instance->hwnd, NULL, TRUE);

    // TODO: remove this, for tests:
    testownik_config conf = {0};
    conf.shuffle_answers = true;
    conf.shuffle_questions = true;
    testownik_start_game(&conf);

    PostMessage(GetParent(instance->hwnd), TM_START_GAME, 0, 0);
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
