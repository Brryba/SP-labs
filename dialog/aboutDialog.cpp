#include "aboutDialog.h"
#include "../resource.h"

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

void ShowAboutDialog(HWND hwndParent) {
    extern HINSTANCE currentInstance; // берём из main.cpp
    DialogBox(currentInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hwndParent, AboutDlgProc);
}

void PaintMainWindow(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HBRUSH hBrush = (HBRUSH) GetStockObject(WHITE_BRUSH);
    FillRect(hdc, &ps.rcPaint, hBrush);
    EndPaint(hwnd, &ps);
}
