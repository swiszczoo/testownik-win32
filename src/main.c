#include <Windows.h>

#include <CommCtrl.h>

#include <memory.h>
#include <string.h>

#include <screen_welcome.h>
#include <testownik.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static screen_welcome welcome_screen;

static HWND status_bar;

void init_comm_ctrl(void)
{
    INITCOMMONCONTROLSEX cctrl;
    cctrl.dwSize = sizeof(cctrl);
    cctrl.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES;

    InitCommonControlsEx(&cctrl);
}

void register_screens(void)
{
    screen_welcome_register();
}

void create_screens(HWND parent)
{
    screen_welcome_create(parent, &welcome_screen);
}

void resize_all_screens(int new_width, int new_height)
{
    SetWindowPos(screen_welcome_hwnd(&welcome_screen), NULL, 0, 0, new_width, new_height, SWP_NOMOVE);
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
    testownik_init();

    HWND hwnd;
    MSG msg;
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TestownikWndClass";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&wcex);

    hwnd = CreateWindow(wcex.lpszClassName,     // window class name
        L"Testownik",                           // window caption
        WS_OVERLAPPEDWINDOW,                    // window style
        CW_USEDEFAULT,                          // initial x position
        CW_USEDEFAULT,                          // initial y position
        1000,                                   // initial x size
        600,                                    // initial y size
        NULL,                                   // parent window handle
        NULL,                                   // window menu handle
        hInstance,                              // program instance handle
        NULL);                                  // creation parameters

    status_bar = CreateWindowEx(NULL, STATUSCLASSNAME, NULL,
        SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

    register_screens();
    create_screens(hwnd);

    testownik_try_load_database();

    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    ExitProcess(0);
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{

    switch (iMsg)
    {
    case WM_CREATE:
        return 0; // return -1 to cancel the creation of the window

    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO minMax = (LPMINMAXINFO)lParam;
        minMax->ptMinTrackSize.x = 750;
        minMax->ptMinTrackSize.y = 500;
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        int statusBarParts[] = {width - 400, width - 200, -1 };
        SendMessage(status_bar, SB_SETPARTS, 3, (LPARAM)statusBarParts);

        SendMessage(status_bar, WM_SIZE, 0, 0);

        RECT status_rect;
        GetWindowRect(status_bar, &status_rect);
        resize_all_screens(width, height - status_rect.bottom + status_rect.top);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
