#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define PARSE_NUMBER(Type, ...) Type CONCAT(num, Type) = va_arg(args, Type); \
int CONCAT(numlen, Type) = CONCAT(getlen_, Type)(CONCAT(num, Type)); \
while (width - CONCAT(numlen, Type) > 0){ \
c = ' '; \
__VA_ARGS__ \
width--; \
} \
if (CONCAT(num, Type) == 0){ \
c = '0'; \
__VA_ARGS__ \
cnt++; \
break; \
} else if (CONCAT(num, Type)< 0){ \
c = '-'; \
__VA_ARGS__ \
cnt++; \
CONCAT(num, Type) = -1 * CONCAT(num, Type); \
} \
int CONCAT(div, Type) = 1; \
while (CONCAT(num, Type) / CONCAT(div, Type) >= 10){ \
  CONCAT(div, Type) *= 10; \
} \
while (CONCAT(div, Type) > 0){ \
  c = CONCAT(num, Type) / CONCAT(div, Type) + '0'; \
  __VA_ARGS__ \
  cnt++; \
  CONCAT(num, Type) %= CONCAT(div, Type); \
  CONCAT(div, Type) /= 10; \
} \
break; \



#define PARSE_ARGS(...) assert(fmt != NULL); \
va_list args; \
va_start(args, fmt); \
int cnt = 0; \
char c; \
while (*fmt != '\0'){ \
  if (*fmt != '%'){ \
      c = *fmt; \
      __VA_ARGS__ \
      cnt++; \
      fmt++; \
      continue; \
  } \
  fmt++; \
  int width = 0; \
  while (*fmt >= '0' && *fmt <='9'){ \
    width = width * 10 + (*fmt - '0'); \
    fmt++; \
  } \
  switch (*fmt) \
  { \
    case 'd': \
      PARSE_NUMBER(int, __VA_ARGS__) \
    case 'u': \
      PARSE_NUMBER(unsigned, __VA_ARGS__) \
    case 's': \
      char *s = va_arg(args, char*); \
      assert(s != NULL); \
      int slen = strlen(s); \
      while (width - slen > 0){ \
        c = ' '; \
        __VA_ARGS__ \
        width--; \
      } \
      while (*s != '\0'){ \
        c = *s; \
        __VA_ARGS__ \
        cnt++; \
        s++; \
      } \
      break; \
    default: \
      assert(0); \
      break; \
  } \
  fmt++; \
} \
va_end(args);

static int getlen_int(int num){
  int numlen = 0; 
  if (num == 0){      
    numlen = 1;
  } else if (num < 0){
    numlen++;
    num = -1 * num;
  }
  while (num){
    num /= 10;
    numlen++;
  }
  return numlen;
}

static int getlen_unsigned(unsigned int num){
  int numlen = 0; 
  if (num == 0){      
    numlen = 1;
  } else if (num < 0){
    numlen++;
    num = -1 * num;
  }
  while (num){
    num /= 10;
    numlen++;
  }
  return numlen;
}

int printf(const char *fmt, ...) {
  //panic("Not implemented");
  PARSE_ARGS(; putch(c);)
  return cnt;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  //panic("Not implemented");
  assert(out != NULL);
  char *p = out;

  PARSE_ARGS(; (*p) = c; p++;)
  *p = '\0';
  return cnt;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
