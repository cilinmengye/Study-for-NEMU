#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

/*
 * 另外, 我们也不希望每次读写操作都需要从头开始. 于是我们需要为每一个已经打开的文件引入偏移量属性open_offset,
 * 来记录目前文件操作的位置. 每次对文件读写了多少个字节, 偏移量就前进多少.
 * 
 * 事实上在真正的操作系统中, 把偏移量放在文件记录表中维护会导致用户程序无法实现某些功能.
 * 由于Nanos-lite是一个精简版的操作系统, 上述问题暂时不会出现, 为了简化实现, 我们还是把偏移量放在文件记录表中进行维护.
 */
typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, invalid_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, invalid_write},
#include "files.h"
};

void init_fs() {
  // TODO: initialize the size of /dev/fb
}


/*
 * 实际上, 操作系统中确实存在不少"没有名字"的文件. 
 * 为了统一管理它们, 我们希望通过一个编号来表示文件, 这个编号就是文件描述符(file descriptor). 
 * 一个文件描述符对应一个正在打开的文件, 由操作系统来维护文件描述符到具体文件的映射. 
 * 
 * 在Nanos-lite中, 由于sfs的文件数目是固定的, 我们可以简单地把文件记录表的下标作为相应文件的文件描述符返回给用户程序. 
 * 在这以后, 所有文件操作都通过文件描述符来标识文件:
 * 
 * 于是我们很自然地通过open()系统调用来打开一个文件, 并返回相应的文件描述符
 * 为了简化实现, 我们允许所有用户程序都可以对所有已存在的文件进行读写, 
 * 这样以后, 我们在实现fs_open()的时候就可以忽略flags和mode了.
 */
int fs_open(const char *pathname, int flags, int mode){
  int i;

  for (i = 0; i < sizeof(file_table) / sizeof(file_table[0]); i++){
    if (strcmp(pathname, file_table[i].name) == 0){
      file_table[i].open_offset = 0;
      return i;
    }
  }
  /*
   * 由于sfs中每一个文件都是固定的, 不会产生新文件,
   * 因此"fs_open()没有找到pathname所指示的文件"属于异常情况, 你需要使用assertion终止程序运行.
   */
  assert(0);
}

/*
 * 另外, 我们也不希望每次读写操作都需要从头开始. 于是我们需要为每一个已经打开的文件引入偏移量属性open_offset, 
 * 来记录目前文件操作的位置. 每次对文件读写了多少个字节, 偏移量就前进多少.
 * 
 * 使用ramdisk_read()和ramdisk_write()来进行文件的真正读写.
 * 由于文件的大小是固定的, 在实现fs_read(), fs_write()和fs_lseek()的时候, 注意偏移量不要越过文件的边界.
 * 除了写入stdout和stderr之外(用putch()输出到串口), 其余对于stdin, stdout和stderr这三个特殊文件的操作可以直接忽略.
 */
size_t fs_read(int fd, void *buf, size_t len){
  size_t ret = ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += ret;
  assert(file_table[fd].open_offset <= file_table[fd].size);
  return ret;
}

/*
 * 除了写入stdout和stderr之外(用putch()输出到串口), 其余对于stdin, stdout和stderr这三个特殊文件的操作可以直接忽略.
 */
size_t fs_write(int fd, const void *buf, size_t len){
  size_t i;
  if (fd == 1 || fd == 2){
    for (i = 0; i < len; i++)
      putch(*(char *)(buf + i));
    return i;
  } 
  size_t ret = ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += ret;
  assert(file_table[fd].open_offset <= file_table[fd].size);
  return ret;
}

/*
 * 由于sfs没有维护文件打开的状态, fs_close()可以直接返回0, 表示总是关闭成功.
 */
int fs_close(int fd){
  file_table[fd].open_offset = 0;
  return 0;
}

#define SEEK_SET 0
#define SEEK_CUR 1 
#define SEEK_END 2
/*
 * 偏移量可以通过lseek()系统调用来调整, 从而可以对文件中的任意位置进行读写:
 * man 2 lseek
 * Upon successful completion, lseek() returns the resulting offset  location  as  measured  
 * in bytes from the beginning of the file.  On error,
 * the value (off_t) -1 is returned and errno is set to indicate  the  error.
 * 
 * whence：参考位置，指定了 offset 是相对于哪个位置计算的。可以取以下三个值之一：
 * SEEK_SET：相对于文件开头。
 * SEEK_CUR：相对于当前文件指针的位置。
 * SEEK_END：相对于文件末尾。
 */
size_t fs_lseek(int fd, size_t offset, int whence){
  switch (whence)
  {
  case SEEK_SET:
    file_table[fd].open_offset = offset;
    break;
  case SEEK_CUR:
    file_table[fd].open_offset += offset;
    assert(file_table[fd].open_offset <=  file_table[fd].size);
    break;
  case SEEK_END:
    file_table[fd].open_offset = file_table[fd].size - offset;
  default:
    assert(0);
    break;
  }
  return file_table[fd].open_offset;
}