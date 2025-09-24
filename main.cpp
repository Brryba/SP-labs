#include <windows.h>
#include "resource.h"
#include "spriteWindow.h"

#define INACTIVITY_TIMER 1
#define INACTIVITY_TIMEOUT 30000

HINSTANCE currentInstance;

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            break;
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            SetTimer(hwnd, INACTIVITY_TIMER, INACTIVITY_TIMEOUT, nullptr);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
            FillRect(hdc, &ps.rcPaint, hBrush);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_TIMER:
            KillTimer(hwnd, INACTIVITY_TIMER);
            showSpriteWindow(currentInstance, hwnd);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_HELP_ABOUT:
                    DialogBox(currentInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, AboutDlgProc);
                    break;
            }
            break;
        case WM_MOUSEMOVE:
        case WM_KEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            KillTimer(hwnd, INACTIVITY_TIMER);
            SetTimer(hwnd, INACTIVITY_TIMER, INACTIVITY_TIMEOUT, NULL);
            break;
        case WM_SPRITE_CLOSED:
            SetTimer(hwnd, INACTIVITY_TIMER, INACTIVITY_TIMEOUT, NULL);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "SimpleWindowClass";

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wc);

    HWND mainWindow = CreateWindowEx(
            0,
            CLASS_NAME,
            "Text editor",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
            NULL, NULL, hInstance, NULL
    );

    if (mainWindow == NULL) return 0;

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
