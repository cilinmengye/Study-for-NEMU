#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>
char handle_key(SDL_Event *ev);

static int cmd_execve(int idx, char **argv);
static int cmd_help(int idx, char **argv);

// 切记这里不能写中文，否则会引起ics2023/navy-apps/apps/nterm/src/main.cpp中
// 参数char ch为负数从而导致段错误
static struct {
  const char *name;
  const char *description;
  int (*handle)(int, char **);
} cmd_table [] = {
  {"nterm", "a simulated terminal", cmd_execve}, 
  {"bmp-test",  "a small test program", cmd_execve},
  {"hello",   "a small test program", cmd_execve},
  {"timer-test", "a small test program", cmd_execve},
  {"nslider","The simplest displayable application in Navy", cmd_execve},
  {"file-test", "a small test program", cmd_execve},
  {"event-test", "a small test program", cmd_execve},
  {"dummy", "a small test program", cmd_execve},
  {"menu",  "Display an application menu", cmd_execve},
  {"bird",  "a game of bird", cmd_execve},
  {"pal",   "a game of pal", cmd_execve},
  {"help", "Shows the commands that nterm can execute", cmd_help},
};

#define NR_CMD sizeof(cmd_table)/sizeof(cmd_table[0])

static void sh_printf(const char *format, ...) {
  static char buf[512] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 512, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}


void parse_cmd(const char *str, char **cmd_out, char ***argv_out) {
    // 先给 str 做一份可写的拷贝
    char *buf = strdup(str);
    if (!buf) { perror("strdup"); exit(1); }

    // 1) 拆出第一个 token —— 指令名
    char *saveptr;
    char *tok = strtok_r(buf, " \t\n", &saveptr);
    if (!tok) {
        *cmd_out = NULL;
        *argv_out = NULL;
        free(buf);
        return;
    }
    *cmd_out = tok;

    // 2) 把后续的 token 丢到 argv 数组里
    //    假设我们不超过 16 个参数
    enum { MAX_ARGV = 16 };
    char **argv = (char **)malloc((MAX_ARGV+1) * sizeof(char*));
    if (!argv) { perror("malloc"); exit(1); }

    int argc = 0;
    argv[argc++] = tok;
    while ((tok = strtok_r(NULL, " \t", &saveptr))) {
        if (argc >= MAX_ARGV) break;
        argv[argc++] = tok;
    }
    argv[argc] = NULL;

    *argv_out = argv;
    // 注意：buf 的内存里同时存着 cmd_out[0]、argv[i] 指向的字符数据；
    // 你要在不再使用时 free(buf) 和 free(argv)。
}

static void sh_handle_cmd(const char *str) {
  // char *clstr = (char *)str;
  // if (clstr[strlen(clstr) - 1] == '\n') clstr[strlen(clstr) - 1] = '\0';
  // char *str_end = clstr + strlen(clstr);
  // char *cmd = strtok(clstr, " ");
  // if (cmd == NULL) return;

  // char *args = cmd + strlen(cmd) + 1;
  // if (args >= str_end) args = NULL;
  // int i;
  int i;
  char *cmd;
  char **argv;
  parse_cmd(str, &cmd, &argv);

  // debug
  // printf("cmd: %s\n", cmd);
  // for (i = 0; argv[i]; i++) printf("argv[%d]: 0x%x %s\n", i, (uintptr_t)argv[i], argv[i]);
  // if (argv[0] == NULL) printf("argv[0]: NULL\n");

  for (i = 0; i < NR_CMD; i++){
    if (strcmp(cmd, cmd_table[i].name) == 0){
      if (cmd_table[i].handle(i, argv) < 0) sh_printf("cmd %s execve fail\n", cmd);
      break;
    }
  }
  if (i == NR_CMD) sh_printf("Unknown command '%s'\n", cmd);
  free((void*)argv);
  free(cmd);
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


static int cmd_execve(int idx, char **argv){
  setenv("PATH", "/bin", 0);
  if (execvp(cmd_table[idx].name,(char * const *)argv) == -1) return -1;
  return 0;
}

static int cmd_help(int idx, char **argv){
  int i;

  if (argv[1] == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
      sh_printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
  }
  else {
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(argv[1], cmd_table[i].name) == 0) {
        sh_printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    sh_printf("Unknown command '%s'\n", argv[1]);
  }
  return 0;
}

