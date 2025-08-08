#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <common.h>

// 红色
#define Log(format, ...) \
  printf("\33[1;35m[%s,%d,%s] " format "\33[0m\n", \
      __FILE__, __LINE__, __func__, ## __VA_ARGS__)

// 打印内核线程的输出：蓝色
#define K_Log(format, ...) \
  printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
      __FILE__, __LINE__, __func__, ## __VA_ARGS__)

// 打印调试与Yield相关的输出：黄色
#define Y_Log(format, ...) \
  printf("\x1b[1;33m[%s,%d,%s] " format "\x1b[0m\n", \
      __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#undef panic
#define panic(format, ...) \
  do { \
    Log("\33[1;31msystem panic: " format, ## __VA_ARGS__); \
    halt(1); \
  } while (0)

#ifdef assert
# undef assert
#endif

#define assert(cond) \
  do { \
    if (!(cond)) { \
      panic("Assertion failed: %s", #cond); \
    } \
  } while (0)

#define Assert(cond, format, ...) \
  do { \
    if (!(cond)) { \
      panic(format, ## __VA_ARGS__);  \
    } \
  } while (0)

#define TODO() panic("please implement me")

#endif
