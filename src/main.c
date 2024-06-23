#include <Windows.h>

#include <CommCtrl.h>

#include <memory.h>
#include <string.h>

#include <image_decoder.h>
#include <messages.h>
#include <random.h>
#include <resource.h>
#include <testownik.h>
#include <theme.h>

#include <screen_ending.h>
#include <screen_question.h>
#include <screen_welcome.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

typedef enum {
    SCREEN_WELCOME,
    SCREEN_QUESTION,
    SCREEN_ENDING,
} screen;

static screen current_screen = -1;
static screen_welcome welcome_screen;
static screen_question question_screen;
static screen_ending ending_screen;

static int dpi;
static HWND status_bar;
static performance_bar perf_bar;

static void init_comm_ctrl(void)
{
    INITCOMMONCONTROLSEX cctrl;
    cctrl.dwSize = sizeof(cctrl);
    cctrl.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES;

    InitCommonControlsEx(&cctrl);
}

static void register_screens(void)
{
    screen_welcome_register();
    screen_question_register();
    screen_ending_register();
}

static void create_screens(HWND parent)
{
    screen_welcome_create(parent, &welcome_screen, status_bar);
    screen_question_create(parent, &question_screen, status_bar, &perf_bar);
    screen_ending_create(parent, &ending_screen, status_bar);
}

static void set_current_screen(screen new_screen)
{
    current_screen = new_screen;

    ShowWindow(screen_welcome_hwnd(&welcome_screen),
        new_screen == SCREEN_WELCOME ? SW_SHOW : SW_HIDE);
    ShowWindow(screen_question_hwnd(&question_screen),
        new_screen == SCREEN_QUESTION ? SW_SHOW : SW_HIDE);
    ShowWindow(performance_bar_hwnd(&perf_bar),
        new_screen == SCREEN_QUESTION ? SW_SHOW : SW_HIDE);
    ShowWindow(screen_ending_hwnd(&ending_screen),
        new_screen == SCREEN_ENDING ? SW_SHOW : SW_HIDE);
}

static void destroy_screens(void)
{
    screen_welcome_destroy(&welcome_screen);
    screen_question_destroy(&question_screen);
    screen_ending_destroy(&ending_screen);
}

static void resize_all_screens(int new_width, int new_height)
{
    SetWindowPos(screen_welcome_hwnd(&welcome_screen), NULL, 0, 0, new_width, new_height, SWP_NOMOVE);
    SetWindowPos(screen_question_hwnd(&question_screen), NULL, 0, 0, new_width, new_height, SWP_NOMOVE);
    SetWindowPos(screen_ending_hwnd(&ending_screen), NULL, 0, 0, new_width, new_height, SWP_NOMOVE);

    RECT status_rect;
    GetClientRect(status_bar, &status_rect);

    int y = (status_rect.bottom - status_rect.top - dip(20)) / 2 + 1;
    int text_width = dip(150);

    SetWindowPos(performance_bar_hwnd(&perf_bar), NULL,
        new_width - dip(300) + text_width, y, dip(300) - text_width - dip(30), dip(20), SWP_NOOWNERZORDER);
}

static HWND current_screen_hwnd(void)
{
    switch (current_screen) {
    case SCREEN_WELCOME: return screen_welcome_hwnd(&welcome_screen);
    case SCREEN_QUESTION: return screen_question_hwnd(&question_screen);
    case SCREEN_ENDING: return screen_ending_hwnd(&ending_screen);
    }

    return NULL;
}


int dip(int input)
{
    static const int DEFAULT_DPI = 120;

    return (input * dpi + dpi / 2) / DEFAULT_DPI;
}

int WINAPI wWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
#ifdef NOCRT
    hInstance = GetModuleHandle(NULL);
    hPrevInstance = NULL;
    lpCmdLine = GetCommandLine();
    nShowCmd = SW_SHOWDEFAULT;
#endif

    init_comm_ctrl();
    random_init();
    testownik_init();

    if (CoInitialize(NULL) != S_OK) {
        MessageBox(NULL,
            L"Nie uda\u0142o si\u0119 zainicjalizowa\u0107 biblioteki COM.",
            NULL, MB_ICONERROR);

        ExitProcess(1);
        return 1;
    }

    image_decoder_init();

    HWND hwnd;
    MSG msg;
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TESTOWNIK));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TestownikWndClass";
    wcex.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TESTOWNIK_SM));

    RegisterClassEx(&wcex);

    dpi = USER_DEFAULT_SCREEN_DPI;

    hwnd = CreateWindow(wcex.lpszClassName,     // window class name
        L"Testownik v" VERSION_STRING,          // window caption
        WS_OVERLAPPEDWINDOW,                    // window style
        300,                                    // initial x position
        200,                                    // initial y position
        dip(1000),                              // initial x size
        dip(800),                               // initial y size
        NULL,                                   // parent window handle
        NULL,                                   // window menu handle
        hInstance,                              // program instance handle
        NULL);                                  // creation parameters


    // Initialize themes
    theme_setup_dark_mode(hwnd);

    HMODULE user32dll = GetModuleHandle(L"user32.dll");
    if (user32dll) {
        UINT(*dpi_fn)(HWND) = (void*)GetProcAddress(user32dll, "GetDpiForWindow");
        if (dpi_fn) {
            dpi = dpi_fn(hwnd);
        }
    }

    status_bar = CreateWindow(STATUSCLASSNAME, NULL,
        SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, hInstance, NULL);
    theme_setup_status_bar(status_bar);

    performance_bar_register();
    performance_bar_create(status_bar, &perf_bar, 0, 0, 0, 0);

    register_screens();
    create_screens(hwnd);
    set_current_screen(SCREEN_WELCOME);

    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        screen_welcome_run(&welcome_screen);

        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    destroy_screens();
    image_decoder_destroy();
    theme_destroy();

    ExitProcess(0);
    return 0;
}

static void event_game_start(void)
{
    set_current_screen(SCREEN_QUESTION);
    screen_question_run(&question_screen);
}

static void event_game_end(void)
{
    set_current_screen(SCREEN_ENDING);
    screen_ending_run(&ending_screen);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
    case WM_CREATE:
        return 0;

    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO minMax = (LPMINMAXINFO)lParam;
        minMax->ptMinTrackSize.x = dip(1200);
        minMax->ptMinTrackSize.y = dip(850);
        return 0;
    }

    case WM_CLOSE:
        if (MessageBox(hwnd, L"Czy na pewno?", L"Wyjd\u017a z Testownika",
            MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {

            DestroyWindow(hwnd);
        }
        return 0;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        int statusBarParts[] = {
            width - dip(900),
            width - dip(700),
            width - dip(500),
            width - dip(300),
            -1
        };
        SendMessage(status_bar, SB_SETPARTS, 5, (LPARAM)statusBarParts);
        SendMessage(status_bar, WM_SIZE, 0, 0);

        RECT status_rect;
        GetWindowRect(status_bar, &status_rect);
        resize_all_screens(width, height - status_rect.bottom + status_rect.top);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case TM_START_GAME:
        event_game_start();
        return 0;

    case TM_END_GAME:
        event_game_end();
        return 0;

    case WM_KEYDOWN:
    {
        return SendMessage(current_screen_hwnd(), iMsg, wParam, lParam);
    }

    case WM_ERASEBKGND:
        return TRUE;

    }

    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
