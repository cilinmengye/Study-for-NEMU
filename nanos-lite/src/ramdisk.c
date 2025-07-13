#include <common.h>

/*
 * 初始化ramdisk. 一般来说, 程序应该存放在永久存储的介质中(比如磁盘). 
 * 但要在NEMU中对磁盘进行模拟是一个略显复杂工作, 
 * 因此先让Nanos-lite把NEMU的一段内存作为磁盘来使用. 
 * 这样的磁盘有一个专门的名字, 叫ramdisk.
 * 可执行文件位于ramdisk偏移为0处, 访问它就可以得到用户程序的第一个字节.
 */

extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;
#define RAMDISK_SIZE ((&ramdisk_end) - (&ramdisk_start))

/* The kernel is monolithic, therefore we do not need to
 * translate the address `buf' from the user process to
 * a physical one, which is necessary for a microkernel.
 */

/* read `len' bytes starting from `offset' of ramdisk into `buf' */
size_t ramdisk_read(void *buf, size_t offset, size_t len) {
  assert(offset + len <= RAMDISK_SIZE);
  //printf("offset: %u\n", offset);
  memcpy(buf, &ramdisk_start + offset, len);
  return len;
}

/* write `len' bytes starting from `buf' into the `offset' of ramdisk */
size_t ramdisk_write(const void *buf, size_t offset, size_t len) {
  assert(offset + len <= RAMDISK_SIZE);
  memcpy(&ramdisk_start + offset, buf, len);
  return len;
}

void init_ramdisk() {
  Log("ramdisk info: start = %p, end = %p, size = %d bytes",
      &ramdisk_start, &ramdisk_end, RAMDISK_SIZE);
}

size_t get_ramdisk_size() {
  return RAMDISK_SIZE;
}
