#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <NDL.h>
#include <BMP.h>
#include <SDL.h>
#include <SDL_bmp.h>

void clean_screen(uint32_t color)
{
  int sw, sh;
  NDL_ScreenWH(&sw, &sh);
  uint32_t *pixels = malloc(sw * sh * sizeof(uint32_t));
  for (int i = 0; i < sh; i++)
	  for (int j = 0; j < sw; j++)
		  pixels[i * sw + j] = color;
  NDL_DrawRect(pixels, 0, 0, sw, sh);
}

int main() {
  NDL_Init(0);
  int w, h;
  void *bmp = BMP_Load("/share/pictures/projectn.bmp", &w, &h);
  assert(bmp);
  // printf("w: %d, h: %d\n", w, h);
  // w = 0;
  // h = 0;
  NDL_OpenCanvas(&w, &h);
  //printf("w: %d, h: %d\n", w, h);
  clean_screen(0xffffffff);  
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
