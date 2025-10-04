#include <windows.h>

#define SPRITE_REPOSITION_TIMEOUT 1
#define INACTIVITY_TIMEOUT 10000

void StartInactivityTimer(HWND hwnd);
void StopInactivityTimer(HWND hwnd);
