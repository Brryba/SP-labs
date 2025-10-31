#include <windows.h>
#include "resource/resource.h"
#include "utils/timerUtils.h"
#include "dialog/aboutDialog.h"
#include "window/spriteWindow.h"
#include <algorithm>
#include <commdlg.h> // ДОБАВЛЕНО: Для диалога открытия файла (GetOpenFileName)

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

// ДОБАВЛЕНО: Функция для обработки открытия файла и заполнения ячеек
void HandleFileOpen(HWND hwnd) {
    OPENFILENAMEA ofn; // A-версия (ANSI) для работы с char* и байтами
    CHAR szFile[MAX_PATH] = {0}; // Буфер для имени файла

    // Инициализируем структуру
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile; // Указываем на наш буфер
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0"; // Фильтры
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; // Файл должен существовать

    // Показываем диалог. Используем A-версию.
    if (GetOpenFileNameA(&ofn) == TRUE) {
        // Пользователь выбрал файл, открываем его
        HANDLE hFile = CreateFileA(ofn.lpstrFile, // A-версия
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

        if (hFile == INVALID_HANDLE_VALUE) {
            MessageBoxA(hwnd, "Cannot open file.", "Error", MB_OK | MB_ICONERROR);
            return;
        }

        // Получаем размер файла
        DWORD dwFileSize = GetFileSize(hFile, NULL);
        if (dwFileSize == INVALID_FILE_SIZE) {
            MessageBoxA(hwnd, "Cannot get file size.", "Error", MB_OK | MB_ICONERROR);
            CloseHandle(hFile);
            return;
        }

        // Если файл пустой, просто очищаем ячейки
        if (dwFileSize == 0) {
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    SetWindowTextA(hEdits[row][col], "");
                }
            }
            CloseHandle(hFile);
            return;
        }

        // Выделяем память (буфер) под содержимое файла
        CHAR* pBuffer = (CHAR*)HeapAlloc(GetProcessHeap(), 0, dwFileSize);
        if (pBuffer == NULL) {
            MessageBoxA(hwnd, "Memory allocation failed.", "Error", MB_OK | MB_ICONERROR);
            CloseHandle(hFile);
            return;
        }

        // Читаем файл в буфер
        DWORD dwBytesRead = 0;
        if (!ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL) || dwBytesRead != dwFileSize) {
            MessageBoxA(hwnd, "Failed to read file.", "Error", MB_OK | MB_ICONERROR);
            HeapFree(GetProcessHeap(), 0, pBuffer);
            CloseHandle(hFile);
            return;
        }

        // Файл больше не нужен
        CloseHandle(hFile);

        // Распределяем данные по 9 ячейкам
        DWORD chunkSize = dwFileSize / 9; // Размер для одной ячейки
        DWORD currentOffset = 0;

        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                int cellIndex = row * 3 + col;
                DWORD currentChunkSize;

                if (cellIndex == 8) {
                    // Последняя ячейка забирает все, что осталось
                    currentChunkSize = dwFileSize - currentOffset;
                } else {
                    currentChunkSize = chunkSize;
                }

                // Выделяем временный буфер для ячейки (+1 для нуль-терминатора \0)
                CHAR* cellBuffer = (CHAR*)HeapAlloc(GetProcessHeap(), 0, currentChunkSize + 1);
                if (cellBuffer) {
                    // Копируем кусок данных из основного буфера
                    memcpy(cellBuffer, pBuffer + currentOffset, currentChunkSize);

                    // Ставим \0, так как SetWindowTextA ожидает C-строку
                    cellBuffer[currentChunkSize] = '\0';

                    // Устанавливаем текст (A-версия)
                    // Байты будут интерпретированы как ANSI-символы
                    SetWindowTextA(hEdits[row][col], cellBuffer);

                    // Освобождаем временный буфер ячейки
                    HeapFree(GetProcessHeap(), 0, cellBuffer);
                }

                currentOffset += currentChunkSize; // Сдвигаем смещение
            }
        }

        // Освобождаем основной буфер
        HeapFree(GetProcessHeap(), 0, pBuffer);
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
                            // ИЗМЕНЕНО: Убран ES_AUTOHSCROLL, оставлен только ES_AUTOVSCROLL
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
            LPMINMAXINFO pMMI = (LPMINMAXINFO)lParam;
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
                // ДОБАВЛЕНО: Обработка нажатия на ID_FILE_OPEN
                case ID_FILE_OPEN:
                    HandleFileOpen(hwnd); // Вызываем нашу новую функцию
                    break;

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