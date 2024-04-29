#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>

SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  return NULL;
}

int SDL_RemoveTimer(SDL_TimerID id) {
  return 1;
}

/* 
 * SDL_GetTicks(): 它和NDL_GetTicks()的功能类似, 但有一个额外的小要求
 * Returns an unsigned 32-bit value 
 * representing the number of milliseconds since the SDL library initialized.
 */
uint32_t SDL_GetTicks() {
  return NDL_GetTicks();
}

void SDL_Delay(uint32_t ms) {
}
