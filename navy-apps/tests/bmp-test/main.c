#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <NDL.h>
#include <BMP.h>
#include <SDL.h>
#include <SDL_bmp.h>

int main() {
  NDL_Init(0);
  int w, h;
  void *bmp = BMP_Load("/share/pictures/projectn.bmp", &w, &h);
  assert(bmp);
  // printf("w: %d, h: %d\n", w, h);
  // w = 0;
  // h = 0;
  NDL_OpenCanvas(&w, &h);
  // printf("w: %d, h: %d\n", w, h);
  NDL_DrawRect(bmp, 0, 0, w, h);
  free(bmp);
  NDL_Quit();
  //printf("Test ends! Spinning...\n");
  while (1){
    SDL_Event e;
    SDL_WaitEvent(&e);
    if (e.type == SDL_KEYDOWN) 
      break;
  }
  return 0;
}
