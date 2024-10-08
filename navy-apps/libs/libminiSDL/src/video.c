#include <NDL.h>
#include <sdl-video.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/*
 * SDL_BlitSurface(): 将一张画布中的指定矩形区域复制到另一张画布的指定位置
 * This assumes that the source and destination rectangles are the same size. 
 * If either srcrect or dstrect are NULL, the entire surface (src or dst) is copied. 
 * The final blit rectangles are saved in srcrect and dstrect after all clipping is performed.
 */
void SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(dst && src);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);
  int sw, dw;
  int sh, dh;
  int sx, dx;
  int sy, dy;
  uint32_t *sp = (uint32_t *)src->pixels;
  uint32_t *dp = (uint32_t *)dst->pixels;

  if (srcrect == NULL && dstrect == NULL) {
    //要将 完全的src画布 复制到 完整的dst画布上
    sw = (int)src->w; 
    sh = (int)src->h; 
    sx = 0; 
    sy = 0;
    dw = (int)dst->w; 
    dh = (int)dst->h;
    dx = 0;
    dy = 0;
    assert(dw == sw && dh == sh);
  } else if (srcrect == NULL && dstrect != NULL){
    //要将 完全的src画布 复制到 dstrect指定的dst画布上
    sw = (int)src->w; 
    sh = (int)src->h; 
    sx = 0; 
    sy = 0;
    //画的高和宽是由源画布决定的
    dw = sw;
    dh = sh;
    dx = (int)dstrect->x;
    dy = (int)dstrect->y;
    //assert(dstrect->w >= sw && dstrect->h >= sh);
  } else if (srcrect != NULL && dstrect == NULL) {
    //要将 srcrect指定的src画布 复制到 完全的dst画布上
    sw = (int)srcrect->w; 
    sh = (int)srcrect->h; 
    sx = 0; 
    sy = 0;
    //画的高和宽是由源画布决定的
    dw = sw;
    dh = sh;
    dx = 0;
    dy = 0;
    //assert(dst->w >= sw && dst->h >= sh);
  } else if (srcrect != NULL && dstrect != NULL){
    //要将 srcrect指定的src画布 复制到 dstrect指定的dst画布上
    sw = (int)srcrect->w; 
    sh = (int)srcrect->h; 
    sx = (int)srcrect->x; 
    sy = (int)srcrect->y; 
    dw = (int)dstrect->w;
    dh = (int)dstrect->h;
    dx = (int)dstrect->x;
    dy = (int)dstrect->y;
    assert(dw == sw && dh == sh);
  }

  for (int i = 0; i < sh; i++)
    for (int j = 0; j < sw; j++)
      dp[(dy + i) * dst->w + dx + j] = sp[(sy + i) * src->w + sx + j];
  NDL_DrawRect(dp, dx, dy, dw, dh);
}

/*
 * SDL_FillRect(): 往画布的指定矩形区域中填充指定的颜色
 * dst	the SDL_Surface structure that is the drawing target
 * rect	the SDL_Rect structure representing the rectangle to fill, or NULL to fill the entire surface
 * color	the color to fill with
 * typedef struct SDL_Rect{
 *  int x, y;
 *  int w, h;
 * } SDL_Rect;
 * typedef struct SDL_Surface {
    Uint32 flags;               // 表面的标志
    SDL_PixelFormat *format;    // 表面的像素格式
    int w, h;                   // 表面的宽度和高度
    int pitch;                  // 表面的行字节数
    void *pixels;               // 指向像素数据的指针
    SDL_Rect clip_rect;         // 裁剪矩形
    int refcount;               // 引用计数
 * } SDL_Surface;
 */
void SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color) {
  assert(dst);
  uint32_t *pixels = (uint32_t *)dst->pixels;
  int w;
  int h;
  int x;
  int y;

  if (dstrect == NULL){
    w = (int)dst->w;
    h = (int)dst->h;
    x = 0; 
    y = 0;
  } else {
    w = (int)dstrect->w;
    h = (int)dstrect->h;
    x = (int)dstrect->x;
    y = (int)dstrect->y;
  }
  color = SDL_MapRGBA(dst->format,color >> 16 & 0xFF, color >> 8 & 0xFF, color & 0xFF, color >> 24 & 0xFF);

  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++)
      pixels[(y + i) * dst->w + x + j] = color;
  NDL_DrawRect(pixels, x, y, w, h);
}

/*
 * SDL的绘图模块引入了一个Surface的概念, 它可以看成一张具有多种属性的画布
 * SDL_UpdateRect()的作用是将画布中的指定矩形区域同步到屏幕上.
 * s 用于更新指定区域的屏幕内容
 * x 和 y：指定要更新的矩形区域的左上角的坐标。
 * w 和 h：指定要更新的矩形区域的宽度和高度。
 */
void SDL_UpdateRect(SDL_Surface *s, int x, int y, int w, int h) {
  assert(s);
  NDL_DrawRect((uint32_t *)s->pixels, x, y, s->w, s->h);
}

// APIs below are already implemented.

static inline int maskToShift(uint32_t mask) {
  switch (mask) {
    case 0x000000ff: return 0;
    case 0x0000ff00: return 8;
    case 0x00ff0000: return 16;
    case 0xff000000: return 24;
    case 0x00000000: return 24; // hack
    default: assert(0);
  }
}

SDL_Surface* SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
    uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  assert(depth == 8 || depth == 32);
  SDL_Surface *s = malloc(sizeof(SDL_Surface));
  assert(s);
  s->flags = flags;
  s->format = malloc(sizeof(SDL_PixelFormat));
  assert(s->format);
  if (depth == 8) {
    s->format->palette = malloc(sizeof(SDL_Palette));
    assert(s->format->palette);
    s->format->palette->colors = malloc(sizeof(SDL_Color) * 256);
    assert(s->format->palette->colors);
    memset(s->format->palette->colors, 0, sizeof(SDL_Color) * 256);
    s->format->palette->ncolors = 256;
  } else {
    s->format->palette = NULL;
    s->format->Rmask = Rmask; s->format->Rshift = maskToShift(Rmask); s->format->Rloss = 0;
    s->format->Gmask = Gmask; s->format->Gshift = maskToShift(Gmask); s->format->Gloss = 0;
    s->format->Bmask = Bmask; s->format->Bshift = maskToShift(Bmask); s->format->Bloss = 0;
    s->format->Amask = Amask; s->format->Ashift = maskToShift(Amask); s->format->Aloss = 0;
  }

  s->format->BitsPerPixel = depth;
  s->format->BytesPerPixel = depth / 8;

  s->w = width;
  s->h = height;
  s->pitch = width * depth / 8;
  assert(s->pitch == width * s->format->BytesPerPixel);

  if (!(flags & SDL_PREALLOC)) {
    s->pixels = malloc(s->pitch * height);
    assert(s->pixels);
  }

  return s;
}

SDL_Surface* SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth,
    int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  SDL_Surface *s = SDL_CreateRGBSurface(SDL_PREALLOC, width, height, depth,
      Rmask, Gmask, Bmask, Amask);
  assert(pitch == s->pitch);
  s->pixels = pixels;
  return s;
}

void SDL_FreeSurface(SDL_Surface *s) {
  if (s != NULL) {
    if (s->format != NULL) {
      if (s->format->palette != NULL) {
        if (s->format->palette->colors != NULL) free(s->format->palette->colors);
        free(s->format->palette);
      }
      free(s->format);
    }
    if (s->pixels != NULL && !(s->flags & SDL_PREALLOC)) free(s->pixels);
    free(s);
  }
}

SDL_Surface* SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
  if (flags & SDL_HWSURFACE) NDL_OpenCanvas(&width, &height);
  return SDL_CreateRGBSurface(flags, width, height, bpp,
      DEFAULT_RMASK, DEFAULT_GMASK, DEFAULT_BMASK, DEFAULT_AMASK);
}

void SDL_SoftStretch(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(src && dst);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);
  assert(dst->format->BitsPerPixel == 8);

  int x = (srcrect == NULL ? 0 : srcrect->x);
  int y = (srcrect == NULL ? 0 : srcrect->y);
  int w = (srcrect == NULL ? src->w : srcrect->w);
  int h = (srcrect == NULL ? src->h : srcrect->h);

  assert(dstrect);
  if(w == dstrect->w && h == dstrect->h) {
    /* The source rectangle and the destination rectangle
     * are of the same size. If that is the case, there
     * is no need to stretch, just copy. */
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_BlitSurface(src, &rect, dst, dstrect);
  }
  else {
    assert(0);
  }
}

void SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int firstcolor, int ncolors) {
  assert(s);
  assert(s->format);
  assert(s->format->palette);
  assert(firstcolor == 0);

  s->format->palette->ncolors = ncolors;
  memcpy(s->format->palette->colors, colors, sizeof(SDL_Color) * ncolors);

  if(s->flags & SDL_HWSURFACE) {
    assert(ncolors == 256);
    for (int i = 0; i < ncolors; i ++) {
      uint8_t r = colors[i].r;
      uint8_t g = colors[i].g;
      uint8_t b = colors[i].b;
    }
    SDL_UpdateRect(s, 0, 0, 0, 0);
  }
}

static void ConvertPixelsARGB_ABGR(void *dst, void *src, int len) {
  int i;
  uint8_t (*pdst)[4] = dst;
  uint8_t (*psrc)[4] = src;
  union {
    uint8_t val8[4];
    uint32_t val32;
  } tmp;
  int first = len & ~0xf;
  for (i = 0; i < first; i += 16) {
#define macro(i) \
    tmp.val32 = *((uint32_t *)psrc[i]); \
    *((uint32_t *)pdst[i]) = tmp.val32; \
    pdst[i][0] = tmp.val8[2]; \
    pdst[i][2] = tmp.val8[0];

    macro(i + 0); macro(i + 1); macro(i + 2); macro(i + 3);
    macro(i + 4); macro(i + 5); macro(i + 6); macro(i + 7);
    macro(i + 8); macro(i + 9); macro(i +10); macro(i +11);
    macro(i +12); macro(i +13); macro(i +14); macro(i +15);
  }

  for (; i < len; i ++) {
    macro(i);
  }
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, uint32_t flags) {
  assert(src->format->BitsPerPixel == 32);
  assert(src->w * src->format->BytesPerPixel == src->pitch);
  assert(src->format->BitsPerPixel == fmt->BitsPerPixel);

  SDL_Surface* ret = SDL_CreateRGBSurface(flags, src->w, src->h, fmt->BitsPerPixel,
    fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

  assert(fmt->Gmask == src->format->Gmask);
  assert(fmt->Amask == 0 || src->format->Amask == 0 || (fmt->Amask == src->format->Amask));
  ConvertPixelsARGB_ABGR(ret->pixels, src->pixels, src->w * src->h);

  return ret;
}

uint32_t SDL_MapRGBA(SDL_PixelFormat *fmt, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  assert(fmt->BytesPerPixel == 4);
  uint32_t p = (r << fmt->Rshift) | (g << fmt->Gshift) | (b << fmt->Bshift);
  if (fmt->Amask) p |= (a << fmt->Ashift);
  return p;
}

int SDL_LockSurface(SDL_Surface *s) {
  return 0;
}

void SDL_UnlockSurface(SDL_Surface *s) {
}
