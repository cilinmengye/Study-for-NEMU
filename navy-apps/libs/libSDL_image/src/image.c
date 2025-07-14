#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

/*
 * 接受一个图片文件的路径, 然后把图片的像素信息封装成SDL的Surface结构并返回. 这个API的一种实现方式如下:
 * 用libc中的文件操作打开文件, 并获取文件大小size
 * 申请一段大小为size的内存区间buf
 * 将整个文件读取到buf中
 * 将buf和size作为参数, 调用STBIMG_LoadFromMemory(), 它会返回一个SDL_Surface结构的指针
 * 关闭文件, 释放申请的内存
 * 返回SDL_Surface结构指针
 */
SDL_Surface* IMG_Load(const char *filename) {
  FILE *file;
  long size;
  SDL_Surface *ret;

  //用libc中的文件操作打开文件
  file = fopen(filename, "rb");
  assert(file != NULL);
  //获取文件大小size
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  //申请一段大小为size的内存区间buf
  unsigned char *buf = (unsigned char *)malloc(size);
  assert(buf != NULL);
  //将整个文件读取到buf中
  fseek(file, 0, SEEK_SET);
  size_t bytesRead = fread(buf, 1, size, file);
  assert(bytesRead == size);
  //将buf和size作为参数, 调用STBIMG_LoadFromMemory(), 它会返回一个SDL_Surface结构的指针
  ret = STBIMG_LoadFromMemory(buf, bytesRead);
  //关闭文件, 释放申请的内存
  fclose(file);
  free(buf);
  return ret;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
