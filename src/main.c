#include <Windows.h>

#include <CommCtrl.h>

#include <memory.h>
#include <string.h>

#include <messages.h>
#include <random.h>
#include <resource.h>
#include <testownik.h>

#include <screen_question.h>
#include <screen_welcome.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

typedef enum {
    SCREEN_WELCOME,
    SCREEN_QUESTION,
} screen;

static screen current_screen = -1;
static screen_welcome welcome_screen;
static screen_question question_screen;

static HWND status_bar;

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
}

static void create_screens(HWND parent)
{
    screen_welcome_create(parent, &welcome_screen, status_bar);
    screen_question_create(parent, &question_screen, status_bar);
}

static void set_current_screen(screen new_screen)
{
    current_screen = new_screen;

    ShowWindow(screen_welcome_hwnd(&welcome_screen),
        new_screen == SCREEN_WELCOME ? SW_SHOW : SW_HIDE);
    ShowWindow(screen_question_hwnd(&question_screen),
        new_screen == SCREEN_QUESTION ? SW_SHOW : SW_HIDE);
}

static void destroy_screens(void)
{
    screen_welcome_destroy(&welcome_screen);
    screen_question_destroy(&question_screen);
}

static void resize_all_screens(int new_width, int new_height)
{
    SetWindowPos(screen_welcome_hwnd(&welcome_screen), NULL, 0, 0, new_width, new_height, SWP_NOMOVE);
    SetWindowPos(screen_question_hwnd(&question_screen), NULL, 0, 0, new_width, new_height, SWP_NOMOVE);
}

static HWND current_screen_hwnd(void)
{
    switch (current_screen) {
    case SCREEN_WELCOME: return screen_welcome_hwnd(&welcome_screen);
    case SCREEN_QUESTION: return screen_question_hwnd(&question_screen);
    }

    return NULL;
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
        MessageBox(NULL, L"Can't initialize COM library. Exiting.", NULL, MB_ICONERROR);
        ExitProcess(1);
        return 1;
    }

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
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TestownikWndClass";
    wcex.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TESTOWNIK_SM));

    RegisterClassEx(&wcex);

    hwnd = CreateWindow(wcex.lpszClassName,     // window class name
        L"Testownik",                           // window caption
        WS_OVERLAPPEDWINDOW,                    // window style
        300,                                    // initial x position
        200,                                    // initial y position
        1000,                                   // initial x size
        800,                                    // initial y size
        NULL,                                   // parent window handle
        NULL,                                   // window menu handle
        hInstance,                              // program instance handle
        NULL);                                  // creation parameters

    status_bar = CreateWindow(STATUSCLASSNAME, NULL,
        SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

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

    ExitProcess(0);
    return 0;
}

static void event_game_start(void)
{
    set_current_screen(SCREEN_QUESTION);
    screen_question_run(&question_screen);
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
        minMax->ptMinTrackSize.x = 1200;
        minMax->ptMinTrackSize.y = 850;
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        int statusBarParts[] = {width - 900, width - 700, width - 500, width - 300, -1 };
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


    case WM_KEYDOWN:
    {
        return SendMessage(current_screen_hwnd(), iMsg, wParam, lParam);
    }

    }

    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
