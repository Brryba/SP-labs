#include <windows.h>
#include "resource.h"
#include "utils/timerUtils.h"
#include "dialog/aboutDialog.h"
#include "spriteWindow.h"

static HINSTANCE currentInstance;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            StartInactivityTimer(hwnd);
            break;
        case WM_PAINT:
            PaintMainWindow(hwnd);
            break;
        case WM_TIMER:
            StopInactivityTimer(hwnd);
            showSpriteWindow(currentInstance, hwnd);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_HELP_ABOUT:
                    ShowAboutDialog(hwnd);
                    break;
            }
            break;
        case WM_MOUSEMOVE:
        case WM_KEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            StartInactivityTimer(hwnd);
            break;
        case WM_SPRITE_CLOSED:
            StartInactivityTimer(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const char CLASS_NAME[] = "SimpleWindowClass";

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wc);

    HWND mainWindow = CreateWindowEx(
            0, CLASS_NAME, "Text editor",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
            NULL, NULL, hInstance, NULL
    );

    if (!mainWindow) return 0;

    currentInstance = hInstance;
    HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINMENU));
    SetMenu(mainWindow, hMenu);
    ShowWindow(mainWindow, nCmdShow);
    UpdateWindow(mainWindow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
