#include "timerUtils.h"

void StartInactivityTimer(HWND hwnd) {
    KillTimer(hwnd, SPRITE_INACTIVE_TIMEOUT);
    SetTimer(hwnd, SPRITE_INACTIVE_TIMEOUT, INACTIVITY_TIMEOUT, NULL);
}

void StopInactivityTimer(HWND hwnd) {
    KillTimer(hwnd, SPRITE_INACTIVE_TIMEOUT);
}
