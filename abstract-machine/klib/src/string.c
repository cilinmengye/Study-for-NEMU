#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  //panic("Not implemented");
  assert(s != NULL);
  size_t len = 0;

  while (*(s + len) != '\0')
    len++;
  return len;
}

char *strcpy(char *dst, const char *src) {
  //panic("Not implemented");
  assert(dst != NULL);
  assert(src != NULL);
  char *p = dst;


  while (*src != '\0'){
    *dst = *src;
    dst++;
    src++;
  }
  *dst = '\0';
  return p;
}

char *strncpy(char *dst, const char *src, size_t n) {
  panic("Not implemented");
}

char *strcat(char *dst, const char *src) {
  //panic("Not implemented");
  assert(dst != NULL);
  assert(src != NULL);
  char *p = dst;

  dst = dst + strlen(dst);
  while (*src != '\0'){
    *dst = *src;
    dst++;
    src++;
  }
  *dst = '\0';
  return p;
}

int strcmp(const char *s1, const char *s2) {
  //panic("Not implemented");
  assert(s1 != NULL);
  assert(s2 != NULL);
  while (*s1 != '\0' && *s2 != '\0'){
    if (*s1 != *s2)
      return *s1 - *s2;
    s1++;
    s2++;
  }
  if (*s1 == '\0' && *s2 == '\0') return 0;
  else if (*s1 == '\0') return -1;
  else return 1;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  //panic("Not implemented");
  assert(s != NULL);
  unsigned char *p = s;
  unsigned char val = (unsigned char)c;
  size_t i;

  for (i = 0; i < n; i++){
    *p = val;
    p++;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  //panic("Not implemented");
  assert(dst != NULL);
  assert(src != NULL);
  unsigned char* d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;
  if (s == d)
    return dst;
  if (s <= d && d <= (s + n)) {
    // 如果有重叠，则需要从后向前复制，以避免数据被覆盖
    d += n - 1;
    s += n - 1;
    while (n--) {
      *d-- = *s--;
    }
  } else {
    while (n--) {
      *d++ = *s++;
    }     
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  panic("Not implemented");
}


/*用于比较两个内存块的内容是否相同。*/
int memcmp(const void *s1, const void *s2, size_t n) {
  //panic("Not implemented");
  assert(s1 != NULL);
  assert(s2 != NULL);
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;
  size_t i;

  for (i = 0; i < n; i++) {
    if (p1[i] < p2[i]) {
      return -1;
    } else if (p1[i] > p2[i]) {
      return 1;
    }
  }
  return 0;
}

#endif
