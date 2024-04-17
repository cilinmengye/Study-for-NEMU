#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define FILL_SPACE(...) \
while (width - numlen > 0){ \
  c = ' '; \
  __VA_ARGS__ \
  width--; \
  cnt++; \
} \

#define PARSE_NUMBER(Type, ...) \
Type CONCAT(num, Type) = va_arg(args, Type); \
numlen = CONCAT(getlen_, Type)(CONCAT(num, Type)); \
FILL_SPACE(__VA_ARGS__) \
if (CONCAT(num, Type) == 0){ \
c = '0'; \
__VA_ARGS__ \
cnt++; \
break; \
} else if (CONCAT(num, Type) < 0){ \
c = '-'; \
__VA_ARGS__ \
cnt++; \
CONCAT(num, Type) = -1 * CONCAT(num, Type); \
} \
div = 1; \
while (CONCAT(num, Type) / div >= 10){ \
  div *= 10; \
} \
while (div > 0){ \
  c = CONCAT(num, Type) / div + '0'; \
  __VA_ARGS__ \
  cnt++; \
  CONCAT(num, Type) %= div; \
  div /= 10; \
} \
break; \

#define PARSE_ARGS(...) assert(fmt != NULL); \
va_list args; \
va_start(args, fmt); \
int div = 1; \
int width = 0; \
int numlen = 0; \
int cnt = 0; \
char c; \
char hexnum[12]; \
while (*fmt != '\0'){ \
  if (*fmt != '%'){ \
      c = *fmt; \
      __VA_ARGS__ \
      cnt++; \
      fmt++; \
      continue; \
  } \
  fmt++; \
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
    case 'x':\
      numlen = get_hex(va_arg(args, unsigned), hexnum); \
      FILL_SPACE(__VA_ARGS__) \
      numlen--; \
      cnt++; \
      while (numlen >= 0){ \
        c = hexnum[numlen]; \
        __VA_ARGS__ \
        numlen--; \
        cnt++; \
      } \
      break; \
    case 's': \
      char *s = va_arg(args, char*); \
      assert(s != NULL); \
      numlen = strlen(s); \
      FILL_SPACE(__VA_ARGS__) \
      while (*s != '\0'){ \
        c = *s; \
        __VA_ARGS__ \
        cnt++; \
        s++; \
      } \
      break; \
    default: \
      debug(*fmt); \
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

static int get_hex(unsigned int num, char *hexnum){
  int idx = 0;
  int remain;
  if (num == 0){
    hexnum[idx++] = '0';
    return idx;
  }
  while (num != 0){
    assert(idx < 12);
    remain = num % 16;
    if (remain < 10)
      hexnum[idx++] =  remain + '0';
    else 
      hexnum[idx++] = remain - 10 + 'a';
    num /= 16;
  }
  return idx;
}

static void debug(char c){
  putch(c);
  putch(' ');
  putch('e');
  putch('r');
  putch('r');
  putch('o');
  putch('r');
  putch('\n');
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
