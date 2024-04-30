#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>
char handle_key(SDL_Event *ev);

static int cmd_execve(int idx, char *arg);
static int cmd_help(int idx, char *arg);

static struct {
  const char *name;
  const char *description;
  int (*handle)(int, char *);
} cmd_table [] = {
  {"nterm", "NTerm (NJU Terminal) NTerm是一个模拟终端, 它实现了终端的基本功能, 包括字符的键入和回退, 以及命令的获取等", cmd_execve}, 
  {"bmp-test", "一个小的测试程序", cmd_execve},
  {"hello", "一个小的测试程序", cmd_execve},
  {"timer-test", "一个小的测试程序", cmd_execve},
  {"nslider","NSlider (NJU Slider) NSlider是Navy中最简单的可展示应用程序, 它是一个支持翻页的幻灯片播放器", cmd_execve},
  {"file-test", "一个小的测试程序", cmd_execve},
  {"event-test", "一个小的测试程序", cmd_execve},
  {"dummy", "一个小的测试程序", cmd_execve},
  {"menu", "MENU (开机菜单) 开机菜单是另一个行为比较简单的程序, 它会展示一个菜单, 用户可以选择运行哪一个程序", cmd_execve},
  {"help", "展示出nterm能够执行的命令", cmd_help},
};

#define NR_CMD sizeof(cmd_table)/sizeof(cmd_table[0])

static void sh_printf(const char *format, ...) {
  static char buf[512] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 512, format, ap);
  va_end(ap);
  printf("buf: %s\n", buf);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

// static char *init_str(const char *str){
//   char *ret = (char *)malloc(strlen(str) * sizeof(char));
//   char *p = ret;
//   for (int i = 0; i < strlen(str) - 1; i++){
//     *p = *(str + i);
//     p++;
//   }
//   *p = '\0';
//   return ret;
// }

static void sh_handle_cmd(const char *str) {
  char *clstr = (char *)str;
  if (clstr[strlen(clstr) - 1] == '\n') clstr[strlen(clstr) - 1] = '\0';
  char *str_end = clstr + strlen(clstr);
  char *cmd = strtok(clstr, " ");
  if (cmd == NULL) return;

  char *args = cmd + strlen(cmd) + 1;
  if (args >= str_end) args = NULL;
  int i;

  for (i = 0; i < NR_CMD; i++){
    if (strcmp(cmd, cmd_table[i].name) == 0){
      if (cmd_table[i].handle(i, args) < 0) return ;
      break;
    }
  }
  if (i == NR_CMD) sh_printf("Unknown command '%s'\n", cmd);
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}

static int cmd_execve(int idx, char *args){
  setenv("PATH", "/bin", 0);
  if (execvp(cmd_table[idx].name,(char * const *)args) == -1) return -1;
  return 0;
}

static int cmd_help(int idx, char *args){
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++) {
      sh_printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
      //sh_printf("???\n");
    }
  }
  else {
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        sh_printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    sh_printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

