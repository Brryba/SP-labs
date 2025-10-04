#include "timerUtils.h"

void StartInactivityTimer(HWND hwnd) {
    KillTimer(hwnd, SPRITE_REPOSITION_TIMEOUT);
    SetTimer(hwnd, SPRITE_REPOSITION_TIMEOUT, INACTIVITY_TIMEOUT, NULL);
}

void StopInactivityTimer(HWND hwnd) {
    KillTimer(hwnd, SPRITE_REPOSITION_TIMEOUT);
}
