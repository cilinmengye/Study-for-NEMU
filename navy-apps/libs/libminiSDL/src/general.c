#include <NDL.h>
#include <SDL.h>
#include <assert.h>
#include <stdlib.h>

extern uint8_t* keystateArray;
extern int numKeys;

int SDL_Init(uint32_t flags) {
  keystateArray = (uint8_t *)malloc(sizeof(uint8_t) * numKeys);
  return NDL_Init(flags);
}

void SDL_Quit() {
  free(keystateArray);
  NDL_Quit();
}

char *SDL_GetError() {
  return "Navy does not support SDL_GetError()";
}

int SDL_SetError(const char* fmt, ...) {
  assert(0);
  return -1;
}

int SDL_ShowCursor(int toggle) {
  assert(0);
  return 0;
}

/*
 * SDL_WM_SetCaption 是 SDL 1.2 中用于设置窗口标题和图标名称（由窗口管理器显示的文字标识）的函数。
 * title：要在窗口标题栏上显示的文字。
 * icon：当窗口被最小化或在任务列表中显示时，由窗口管理器用来标识该窗口的文字（并非像素图标）。
 * 目前没有想到办法实现，好像没有实现也没有什么大影响
 */
void SDL_WM_SetCaption(const char *title, const char *icon) {
  //assert(0);
}
