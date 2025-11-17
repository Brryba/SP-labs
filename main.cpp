#include <windows.h>
#include "resource/resource.h"
#include "utils/timerUtils.h"
#include "dialog/aboutDialog.h"
#include "window/spriteWindow.h"
#include <commdlg.h>
#include <wchar.h>


#define MIN_WINDOW_WIDTH 500
#define MIN_WINDOW_HEIGHT 400

static HINSTANCE currentInstance;
static HWND hEdits[3][3];
static WNDPROC OriginalEditProc;
static HFONT hCurrentFont = NULL;


void ApplyFontToControls(HFONT hNewFont) {
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (hEdits[row][col]) {
                SendMessage(hEdits[row][col], WM_SETFONT, (WPARAM) hNewFont, TRUE);
            }
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

BOOL GetOpenFilePath(HWND hwnd, WCHAR *file, DWORD maxLen) {
    OPENFILENAMEW ofn = {sizeof(ofn)};
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = file;
    ofn.nMaxFile = maxLen;

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    return GetOpenFileNameW(&ofn);
}

BOOL GetSaveFilePath(HWND hwnd, WCHAR *file, DWORD maxLen) {
    OPENFILENAMEW ofn = {sizeof(ofn)};
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = file;
    ofn.nMaxFile = maxLen;

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    return GetSaveFileNameW(&ofn);
}

BOOL ReadFileToBuffer(LPCWSTR path, CHAR **buf, DWORD *size) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) return FALSE;

    *size = GetFileSize(h, 0);
    if (*size == INVALID_FILE_SIZE) {
        CloseHandle(h);
        return FALSE;
    }

    *buf = static_cast<CHAR *>(HeapAlloc(GetProcessHeap(), 0, *size));
    if (!*buf) {
        CloseHandle(h);
        return FALSE;
    }

    DWORD read = 0;

    BOOL ok = ReadFile(h, *buf, *size, &read, 0);
    CloseHandle(h);

    return ok && read == *size;
}

LPCWSTR MyPathFindExtensionW(LPCWSTR pszPath) {
    if (!pszPath) return NULL;

    LPCWSTR pszDot = wcsrchr(pszPath, L'.');

    if (!pszDot) {
        return pszPath + wcslen(pszPath);
    }

    LPCWSTR pszSlash = wcsrchr(pszPath, L'\\');
    LPCWSTR pszFwdSlash = wcsrchr(pszPath, L'/');

    if (pszSlash == NULL) pszSlash = pszFwdSlash;
    else if (pszFwdSlash && pszFwdSlash > pszSlash) pszSlash = pszFwdSlash;

    if (pszSlash && pszSlash > pszDot) {
        return pszPath + wcslen(pszPath);
    }

    return pszDot;
}


void DistributeBufferToCells(LPCWSTR path, CHAR *buf, DWORD size) {
    if (!buf || !size) {
        for (int r = 0; r < 3; r++)
            for (int c = 0; c < 3; c++)
                SetWindowTextW(hEdits[r][c], L"");
        return;
    }

    BOOL isTxtFile = FALSE;
    LPCWSTR extension = MyPathFindExtensionW(path);
    if (extension && lstrcmpiW(extension, L".txt") == 0) {
        isTxtFile = TRUE;
    }


    if (isTxtFile) {
        DWORD chunk = size / 9, pos = 0;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                DWORD cur = (r * 3 + c == 8) ? (size - pos) : chunk;
                DWORD end = pos + cur;

                while (end < size && (buf[end] & 0xC0) == 0x80) end++;
                cur = end - pos;

                int len = MultiByteToWideChar(CP_UTF8, 0, buf + pos, cur, 0, 0);
                WCHAR *w = static_cast<WCHAR *>(HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR)));

                if (w) {
                    MultiByteToWideChar(CP_UTF8, 0, buf + pos, cur, w, len);
                    w[len] = 0;
                    SetWindowTextW(hEdits[r][c], w);
                    HeapFree(GetProcessHeap(), 0, w);
                }
                pos += cur;
            }
        }

    } else {
        DWORD chunk = size / 9;
        DWORD pos = 0;

        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {

                DWORD cur = (r * 3 + c == 8) ? (size - pos) : chunk;
                if (pos + cur > size) cur = size - pos;
                if (cur == 0 && pos < size && (r*3+c == 8)) cur = size - pos;

                if (cur == 0) {
                    SetWindowTextW(hEdits[r][c], L"");
                    continue;
                }

                int len = MultiByteToWideChar(CP_ACP, 0, buf + pos, cur, 0, 0);

                WCHAR *w = static_cast<WCHAR *>(HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR)));
                if (w) {
                    MultiByteToWideChar(CP_ACP, 0, buf + pos, cur, w, len);
                    w[len] = 0;

                    for (int i = 0; i < len; i++) {
                        if (w[i] == L'\0') {
                            w[i] = L' ';
                        }
                    }

                    SetWindowTextW(hEdits[r][c], w);
                    HeapFree(GetProcessHeap(), 0, w);
                }
                pos += cur;
            }
        }
    }
}


BOOL CollectCellsToBuffer(CHAR **buf, DWORD *totalSize) {
    *buf = NULL;
    *totalSize = 0;

    DWORD currentWCharBufSize = 10 * 1024 * 1024;
    WCHAR *allText = static_cast<WCHAR *>(HeapAlloc(GetProcessHeap(), 0, currentWCharBufSize * sizeof(WCHAR)));

    if (!allText) return FALSE;

    DWORD wcharPos = 0;

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            int len = GetWindowTextLengthW(hEdits[r][c]);
            if (len > 0) {
                if (wcharPos + len + 1 > currentWCharBufSize) {
                    HeapFree(GetProcessHeap(), 0, allText);
                    return FALSE;
                }
                GetWindowTextW(hEdits[r][c], allText + wcharPos, len + 1);
                wcharPos += len;
            }
        }
    }

    if (wcharPos == 0) {
        HeapFree(GetProcessHeap(), 0, allText);
        *buf = NULL;
        *totalSize = 0;
        return TRUE;
    }

    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, allText, wcharPos, 0, 0, 0, 0);
    if (utf8Size <= 0) {
        HeapFree(GetProcessHeap(), 0, allText);
        return FALSE;
    }

    *buf = static_cast<CHAR *>(HeapAlloc(GetProcessHeap(), 0, utf8Size));
    if (!*buf) {
        HeapFree(GetProcessHeap(), 0, allText);
        return FALSE;
    }

    WideCharToMultiByte(CP_UTF8, 0, allText, wcharPos, *buf, utf8Size, 0, 0);

    *totalSize = utf8Size;
    HeapFree(GetProcessHeap(), 0, allText);
    return TRUE;
}

BOOL WriteBufferToFile(LPCWSTR path, CHAR *buf, DWORD size) {
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) return FALSE;

    if (size == 0 || buf == NULL) {
        CloseHandle(h);
        return TRUE;
    }

    DWORD written = 0;

    BOOL ok = WriteFile(h, buf, size, &written, 0);
    CloseHandle(h);

    return ok && written == size;
}

void HandleFileOpen(HWND hwnd) {
    WCHAR file[MAX_PATH] = L"";
    CHAR *buf = NULL;
    DWORD size = 0;

    if (GetOpenFilePath(hwnd, file, MAX_PATH)) {
        if (ReadFileToBuffer(file, &buf, &size)) {
            DistributeBufferToCells(file, buf, size);

            if(buf) {
                HeapFree(GetProcessHeap(), 0, buf);
            }
        } else {
            MessageBoxW(hwnd, L"Не удалось прочитать файл.", L"Ошибка", MB_OK | MB_ICONERROR);
        }
    }
}

void HandleFileSave(HWND hwnd) {
    WCHAR file[MAX_PATH] = L"";
    CHAR *buf = NULL;
    DWORD size = 0;

    if (CollectCellsToBuffer(&buf, &size)) {

        if (GetSaveFilePath(hwnd, file, MAX_PATH)) {

            if (!WriteBufferToFile(file, buf, size)) {
                MessageBoxW(hwnd, L"Не удалось сохранить файл.", L"Ошибка", MB_OK | MB_ICONERROR);
            }
        }
    } else {
        MessageBoxW(hwnd, L"Не удалось собрать данные для сохранения (возможно, слишком большой объем).", L"Ошибка", MB_OK | MB_ICONERROR);
    }

    if (buf) {
        HeapFree(GetProcessHeap(), 0, buf);
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
                            ES_AUTOVSCROLL,
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
        case WM_GETMINMAXINFO: {
            LPMINMAXINFO pMMI = (LPMINMAXINFO) lParam;
            pMMI->ptMinTrackSize.x = MIN_WINDOW_WIDTH;
            pMMI->ptMinTrackSize.y = MIN_WINDOW_HEIGHT;
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
                case ID_FILE_OPEN:
                    HandleFileOpen(hwnd);
                    break;
                case ID_FILE_SAVE:
                    HandleFileSave(hwnd);
                    break; // ИСПРАВЛЕНИЕ: Отсутствовал break
                case ID_HELP_ABOUT:
                    ShowAboutDialog(hwnd, currentInstance);
                    break;
                case FIXEDSYS_FONT:
                case ARIAL_FONT:
                case COURIER_NEW_FONT: {
                    if (hCurrentFont) {
                        DeleteObject(hCurrentFont);
                    }
                    int commandId = LOWORD(wParam);
                    hCurrentFont = CreateApplicationFont(commandId);
                    if (hCurrentFont) {
                        ApplyFontToControls(hCurrentFont);
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
            if (hCurrentFont) {
                DeleteObject(hCurrentFont);
            }
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

