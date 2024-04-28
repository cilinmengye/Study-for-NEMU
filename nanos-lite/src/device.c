#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

/*
 * 由于串口是一个字符设备, 对应的字节序列没有"位置"的概念, 因此serial_write()中的offset参数可以忽略
 */
size_t serial_write(const void *buf, size_t offset, size_t len) {
  size_t i;
  for (i = 0; i < len; i++)
    putch(*(char *)(buf + i));
  return i;
}

/*
 * 把事件写入到buf中, 最长写入len字节, 然后返回写入的实际长度. 
 * 其中按键名已经在字符串数组names中定义好了, 你需要借助IOE的API来获得设备的输入.
 * 另外, 若当前没有有效按键, 则返回0即可.
 * 
 * 另一个输入设备是键盘, 按键信息对系统来说本质上就是到来了一个事件. 一种简单的方式是把事件以文本的形式表现出来, 我们定义以下两种事件,
 * 按下按键事件, 如kd RETURN表示按下回车键
 * 松开按键事件, 如ku A表示松开A键
 * 按键名称与AM中的定义的按键名相同, 均为大写. 此外, 一个事件以换行符\n结束.
 * 
 * 我们可以假设一次最多只会读出一个事件, 这样可以简化你的实现
 */
size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode == AM_KEY_NONE)
    return 0;
  size_t ret = snprintf(buf, len, "%s %s\n", ev.keydown?"kd":"ku", keyname[ev.keycode]);
  return ret;
}

/*
 * 屏幕大小的信息通过/proc/dispinfo文件来获得, 它需要支持读操作. navy-apps/README.md中对这个文件内容的格式进行了约定, 
 * 你需要阅读它. 至于具体的屏幕大小, 你需要通过IOE的相应API来获取.
 * 实现dispinfo_read()(在nanos-lite/src/device.c中定义),
 * 按照约定将文件的len字节写到buf中(我们认为这个文件不支持lseek, 可忽略offset).
 */
size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  int w = io_read(AM_GPU_CONFIG).width;
  int h = io_read(AM_GPU_CONFIG).height;
  size_t ret = snprintf(buf, len, "%s:%d\n%s:%d\n", "WIDTH", w, "HEIGHT", h);
  return ret;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
