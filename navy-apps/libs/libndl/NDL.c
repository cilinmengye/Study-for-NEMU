#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
int open(const char *path, int flags, ...);

/*
 * 为了更好地封装IOE的功能, 我们在Navy中提供了一个叫NDL(NJU DirectMedia Layer)的多媒体库. 
 * 这个库的代码位于navy-apps/libs/libndl/NDL.c中
 * 但大部分的功能都没有实现. 代码中有一些和NWM_APP相关的内容, 你目前可以忽略它们, 但不要修改相关代码, 你将会在PA4的最后体验相关的功能.
 */

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static struct timeval NDL_startTime;


/*
 * 你需要用gettimeofday()实现NDL_GetTicks(), 然后修改timer-test测试, 
 * 让它通过调用NDL_GetTicks()来获取当前时间. 你可以根据需要在NDL_Init()和NDL_Quit()中添加初始化代码和结束代码, 
 * 我们约定程序在使用NDL库的功能之前必须先调用NDL_Init(). 
 */
uint32_t NDL_GetTicks() {
  struct timeval now;
  gettimeofday(&now, NULL);
  return (now.tv_sec * 1000000 + now.tv_usec) - (NDL_startTime.tv_sec * 1000000 + NDL_startTime.tv_usec);
}

/*
 * 读出一条事件信息, 将其写入`buf`中, 最长写入`len`字节
 * 若读出了有效的事件, 函数返回1, 否则返回0
 */
int NDL_PollEvent(char *buf, int len) {
  int ret; 
  int fd = open("/dev/events", 0);
  ret = read(fd, buf, len);
  close(fd);
  return ret;
}

/* 
 * 代码中有一些和NWM_APP相关的内容, 你目前可以忽略它们, 但不要修改相关代码, 你将会在PA4的最后体验相关的功能
 * 在NDL中读出这个文件的内容, 从中解析出屏幕大小, 然后实现NDL_OpenCanvas()的功能. 
 * 目前NDL_OpenCanvas()只需要记录画布的大小就可以了, 当然我们要求画布大小不能超过屏幕大小.
 * 
 * 打开一张(*w) X (*h)的画布
 * 如果*w和*h均为0, 则将系统全屏幕作为画布, 并将*w和*h分别设为系统屏幕的大小
 */
void NDL_OpenCanvas(int *w, int *h) {
  int fd = open("/proc/dispinfo", 0);
  char dispinfo_buf[64];
  read(fd, dispinfo_buf, 64);
  sscanf(dispinfo_buf, "WIDTH:%d\nHEIGHT:%d\n", &screen_w, &screen_h);
  if (*w == 0 && *h == 0){
    *w = screen_w;
    *h = screen_h;
  }
  if (*w > screen_w) *w = screen_w;
  if (*h > screen_h) *h = screen_h;
  close(fd);

  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
}

/*
 * 向画布`(x, y)`坐标处绘制`w*h`的矩形图像, 并将该绘制区域同步到屏幕上
 * 图像像素按行优先方式存储在`pixels`中, 每个像素用32位整数以`00RRGGBB`的方式描述颜色
 */
void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  printf("???\n");
  int fd = open("/dev/fb", 0);
  printf("???\n");
  //得到在屏幕上,让画布居中的左上角点
  assert(screen_h >= h);
  assert(screen_w >= w);
  int my = (screen_h - h) / 2;
  int mx = (screen_w - w) / 2;
  printf("NDL_DrawRect fd:%d\n", fd);
  
  for (y = 0; y < h; y++){
    lseek(fd, (y + my) * screen_w + mx, SEEK_SET);
    write(fd, (void *)pixels[y * w], w);
  }
  close(fd);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  gettimeofday(&NDL_startTime, NULL);

  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  return 0;
}

void NDL_Quit() {
}
