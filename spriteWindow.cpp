#include "spriteWindow.h"

#define IDT_ANIMATION_TIMER 2

static HWND g_hParentWnd = NULL;
static int spriteX = 100, spriteY = 100;
static int speedX = 8, speedY = 8;
static POINT lastMouse = { -1, -1 };

LRESULT CALLBACK ScreensaverWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            ShowCursor(FALSE);
            SetTimer(hwnd, IDT_ANIMATION_TIMER, 30, NULL);
            return 0;

        case WM_TIMER:
            if (wParam == IDT_ANIMATION_TIMER) {
                RECT clientRect;
                GetClientRect(hwnd, &clientRect);

                spriteX += speedX;
                spriteY += speedY;

                if (spriteX <= 0 || spriteX + 50 >= clientRect.right) speedX = -speedX;
                if (spriteY <= 0 || spriteY + 50 >= clientRect.bottom) speedY = -speedY;

                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            // 1. Заливаем фон чёрным
            FillRect(hdc, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

            // 2. Рисуем спрайт (зелёный квадрат)
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 0));
            RECT spriteRect = {spriteX, spriteY, spriteX + 50, spriteY + 50};
            FillRect(hdc, &spriteRect, hBrush);
            DeleteObject(hBrush);

            EndPaint(hwnd, &ps);
        }
            return 0;

        case WM_MOUSEMOVE: {
            POINT pt;
            GetCursorPos(&pt);
            if (lastMouse.x == -1 && lastMouse.y == -1) {
                lastMouse = pt; // инициализация
            } else if (pt.x != lastMouse.x || pt.y != lastMouse.y) {
                DestroyWindow(hwnd);
            }
            return 0;
        }
        case WM_KEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            ShowCursor(TRUE);
            KillTimer(hwnd, IDT_ANIMATION_TIMER);
            PostMessage(g_hParentWnd, WM_SPRITE_CLOSED, 0, 0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void showSpriteWindow(HINSTANCE hInst, HWND hParent) {
    g_hParentWnd = hParent;
    const char CLASS_NAME[] = "spriteWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = ScreensaverWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wc.hCursor = NULL;

    RegisterClass(&wc);

    RECT rc;
    GetClientRect(hParent, &rc);

    HWND hwnd = CreateWindowEx(
            WS_EX_TOPMOST, // Поверх всех окон
            CLASS_NAME,
            "Screensaver",
            WS_POPUP,
            0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
            NULL, NULL, hInst, NULL
    );

    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
}