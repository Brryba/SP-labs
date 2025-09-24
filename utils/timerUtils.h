#include <windows.h>

#define INACTIVITY_TIMER 1
#define INACTIVITY_TIMEOUT 30000

void StartInactivityTimer(HWND hwnd);
void StopInactivityTimer(HWND hwnd);
