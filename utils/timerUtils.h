#include <windows.h>

#define SPRITE_INACTIVE_TIMEOUT 1
#define INACTIVITY_TIMEOUT 30000

void StartInactivityTimer(HWND hwnd);
void StopInactivityTimer(HWND hwnd);
