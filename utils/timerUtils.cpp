#include "timerUtils.h"

void StartInactivityTimer(HWND hwnd) {
    KillTimer(hwnd, INACTIVITY_TIMER);
    SetTimer(hwnd, INACTIVITY_TIMER, INACTIVITY_TIMEOUT, NULL);
}

void StopInactivityTimer(HWND hwnd) {
    KillTimer(hwnd, INACTIVITY_TIMER);
}
