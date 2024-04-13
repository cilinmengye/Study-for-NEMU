#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  //panic("Not implemented");
  assert(out != NULL);
  assert(fmt != NULL);
  va_list args;
  va_start(args, fmt);
  char *p = out;

  while (*fmt != '\0'){
    if (*fmt != '%'){
      *p = *fmt;
      p++;
      fmt++;
      continue;
    }
    /*the condition that *fmt == '%'*/
    fmt++;
    switch (*fmt)
    {
    case 'd':
      int num = va_arg(args, int);
      if (num == 0){
        *p = '0';
        p++;
        break;
      } else if (num < 0){
        *p = '-';
        p++;
        num = -1 * num;
      }
      int div = 1;
      while (num / div >= 10){
        div *= 10;
      }
      while (div > 0){
        *p = num / div + '0';
        num %= div;
        div /= 10;
      }
      break;
    case 's':
      char *s = va_arg(args, char*);
      assert(s != NULL);
      while (*s != '\0'){
        *p = *s;
        p++;
        s++;
      }
      break;
    default:
      assert(0);
      break;
    }
  }
  *p = '\0';
  va_end(args);
  /*len is not include '\0'*/
  return p - out - 1;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
