#include <windows.h>
#include "resource/resource.h"
#include "utils/timerUtils.h"
#include "dialog/aboutDialog.h"
#include "window/spriteWindow.h"

static HINSTANCE currentInstance;
static HWND hEdits[3][3];
static WNDPROC OriginalEditProc;
static HFONT hCurrentFont = NULL;

void ApplyFontToControls(HFONT hNewFont) {
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            SendMessage(hEdits[row][col], WM_SETFONT, (WPARAM) hNewFont, TRUE);
        }
    }
}

LRESULT CALLBACK EditWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN: {
            HWND hParent = GetParent(hwnd);
            if (hParent) {
                StartInactivityTimer(hParent);
            }
            break;
        }
    }
    return CallWindowProc(OriginalEditProc, hwnd, uMsg, wParam, lParam);
}

HFONT CreateApplicationFont(int fontId) {
    switch (fontId) {
        case FIXEDSYS_FONT:
            return CreateFont(25, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, FIXED_PITCH, TEXT("Fixedsys"));

        case ARIAL_FONT:
            return CreateFont(25, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, TEXT("Arial"));

        case COURIER_NEW_FONT:
            return CreateFont(25, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("Courier New"));

        default:
            return NULL;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            StartInactivityTimer(hwnd);

            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    hEdits[row][col] = CreateWindowEx(
                            WS_EX_CLIENTEDGE, "EDIT", "",
                            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_WANTRETURN |
                            ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                            0, 0, 0, 0, hwnd,
                            (HMENU) (1000 + row * 3 + col),
                            ((LPCREATESTRUCT) lParam)->hInstance, NULL);

                    if (row == 0 && col == 0) {
                        OriginalEditProc = (WNDPROC) GetWindowLongPtr(hEdits[row][col], GWLP_WNDPROC);
                    }
                    SetWindowLongPtr(hEdits[row][col], GWLP_WNDPROC, (LONG_PTR) EditWindowProc);
                }
            }
            hCurrentFont = CreateApplicationFont(FIXEDSYS_FONT);
            ApplyFontToControls(hCurrentFont);
            break;
        }

        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            int cellW = width / 3;
            int cellH = height / 3;

            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    MoveWindow(hEdits[row][col], col * cellW, row * cellH, cellW, cellH, TRUE);
                }
            }
            break;
        }

        case WM_PAINT:
            PaintMainWindow(hwnd);
            break;

        case WM_TIMER:
            StopInactivityTimer(hwnd);
            showSpriteWindow(currentInstance, hwnd);
            break;

        case WM_COMMAND: {
            if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) >= 1000 && LOWORD(wParam) < 1009) {
                StartInactivityTimer(hwnd);
            }

            switch (LOWORD(wParam)) {
                case ID_HELP_ABOUT:
                    ShowAboutDialog(hwnd, currentInstance);
                    break;

                case FIXEDSYS_FONT:
                case ARIAL_FONT:
                case COURIER_NEW_FONT: {
                    if (hCurrentFont) {
                        DeleteObject(hCurrentFont);
                        hCurrentFont = NULL;
                    }

                    HFONT hNewFont = NULL;
                    int commandId = LOWORD(wParam);
                    hNewFont = CreateApplicationFont(commandId);

                    if (hNewFont) {
                        if (commandId != FIXEDSYS_FONT) {
                            hCurrentFont = hNewFont;
                        }
                        ApplyFontToControls(hNewFont);
                    }
                    break;
                }
            }
            break;
        }

        case WM_MOUSEMOVE:
        case WM_KEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            StartInactivityTimer(hwnd);
            break;

        case WM_SPRITE_CLOSED:
            StartInactivityTimer(hwnd);
            ShowCursor(TRUE);
            break;

        case WM_DESTROY: {
            DeleteObject(hCurrentFont);
            PostQuitMessage(0);
            return 0;
        }
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
            0, CLASS_NAME, "Text Table 3x3",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
            NULL, NULL, hInstance, NULL);

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