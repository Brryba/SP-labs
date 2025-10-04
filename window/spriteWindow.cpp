#include "spriteWindow.h"

#define SPRITE_REPOSITION_TIMEOUT 2
#define SPRITE_SIZE 50
#define TIMER_INTERVAL 30
#define MOVE_STEP 5

static HWND g_hParentWnd = NULL;
static int spriteCurrentX = 100, spriteCurrentY = 100;
static int defaultSpeedX = 8, defaultSpeedY = 8;
static POINT lastMouse = {-1, -1};
static void UpdateSpritePosition(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    spriteCurrentX += defaultSpeedX;
    spriteCurrentY += defaultSpeedY;

    if (spriteCurrentX <= 0 || spriteCurrentX + SPRITE_SIZE >= rc.right) defaultSpeedX = -defaultSpeedX;
    if (spriteCurrentY <= 0 || spriteCurrentY + SPRITE_SIZE >= rc.bottom) defaultSpeedY = -defaultSpeedY;

    InvalidateRect(hwnd, NULL, FALSE);
}

static void PaintSprite(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

    FillRect(memDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 0));
    RECT spriteRect = {
            spriteCurrentX,
            spriteCurrentY,
            spriteCurrentX + SPRITE_SIZE,
            spriteCurrentY + SPRITE_SIZE
    };
    FillRect(memDC, &spriteRect, hBrush);
    DeleteObject(hBrush);

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    EndPaint(hwnd, &ps);
}


static void MoveSpriteByKey(HWND hwnd, WPARAM key) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    KillTimer(hwnd, SPRITE_REPOSITION_TIMEOUT);

    switch (key) {
        case 'W':
        case VK_UP:
            spriteCurrentY -= MOVE_STEP;
            break;
        case 'S':
        case VK_DOWN:
            spriteCurrentY += MOVE_STEP;
            break;
        case 'A':
        case VK_LEFT:
            spriteCurrentX -= MOVE_STEP;
            break;
        case 'D':
        case VK_RIGHT:
            spriteCurrentX += MOVE_STEP;
            break;
    }

    if (spriteCurrentX < 0) spriteCurrentX = 0;
    if (spriteCurrentY < 0) spriteCurrentY = 0;
    if (spriteCurrentX + SPRITE_SIZE > rc.right) spriteCurrentX = rc.right - SPRITE_SIZE;
    if (spriteCurrentY + SPRITE_SIZE > rc.bottom) spriteCurrentY = rc.bottom - SPRITE_SIZE;

    InvalidateRect(hwnd, NULL, FALSE);
}

static LRESULT CALLBACK ScreensaverWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            ShowCursor(FALSE);
            SetTimer(hwnd, SPRITE_REPOSITION_TIMEOUT, TIMER_INTERVAL, NULL);
            return 0;

        case WM_TIMER:
            if (wParam == SPRITE_REPOSITION_TIMEOUT) {
                UpdateSpritePosition(hwnd);
            }
            return 0;

        case WM_PAINT:
            PaintSprite(hwnd);
            return 0;

        case WM_KEYDOWN:
            switch (wParam) {
                case 'W':
                case 'A':
                case 'S':
                case 'D':
                case VK_UP:
                case VK_DOWN:
                case VK_RIGHT:
                case VK_LEFT:
                    MoveSpriteByKey(hwnd, wParam);
                    return 0;
                default:
                    DestroyWindow(hwnd);
                    return 0;
            }

        case WM_MOUSEMOVE: {
            POINT pt;
            GetCursorPos(&pt);
            if (lastMouse.x == -1 && lastMouse.y == -1) {
                lastMouse = pt;
            } else if (pt.x != lastMouse.x || pt.y != lastMouse.y) {
                DestroyWindow(hwnd);
            }
            return 0;
        }

        case WM_LBUTTONDOWN:
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, SPRITE_REPOSITION_TIMEOUT);
            lastMouse = {-1, -1};
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
    POINT pt = {rc.left, rc.top};
    ClientToScreen(hParent, &pt);

    HWND hOverlay = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            CLASS_NAME,
            "Text editor inactive",
            WS_POPUP,
            pt.x, pt.y,
            rc.right - rc.left, rc.bottom - rc.top,
            hParent,
            NULL, hInst, nullptr);

    ShowWindow(hOverlay, SW_SHOW);
    UpdateWindow(hOverlay);
}
