/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Let the program step through N instructions and then pause execution. When N is not given, the default is 1", cmd_si },
  { "info", "info r print register status; info w print monitoring point information", cmd_info },
  { "x", "x N EXPR, Find the value of the expression EXPR and use the result as the starting memory Address, output N consecutive 4 bytes in hexadecimal form", cmd_x },
  { "p", "p EXPR, Find the value of the expression EXPR", cmd_p },
 
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    printf("main_loop: str:%s, cmd:%s, args:%s\n", str, cmd, args);
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}

/*非框架代码，是我为了方便打印命令格式错误的提醒*/
static void cmd_error_help(char *arg){
  int i;

  for (i = 0; i < NR_CMD; i ++) {
    if (strcmp(arg, cmd_table[i].name) == 0) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
      break;
    }
  }
}

static int cmd_si(char *args){
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  /* Number of single-step execution instructions */
  int i;

  if (arg == NULL) {
    /* no argument given */
    i = 1;
  }
  else {
    sscanf (arg, "%d", &i);
  }
  cpu_exec(i);
  return 0;
}

static int cmd_info(char *args){
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (arg == NULL || (strcmp(arg, "r") != 0 && strcmp(arg, "w") != 0)){
    /* if have not subarg or subarg is wrong, I need notice user */
    cmd_error_help("info");
  } else if (strcmp(arg, "r") == 0){
    isa_reg_display();
  } else {
  }
  return 0;
}

static int cmd_x(char *args){
  char *arg1 = strtok(NULL, " ");
  char *arg2;
  char *str_end = args + strlen(args);
  bool success = true;
  int cnt = 0;
  word_t i;
  word_t n;
  word_t exprAddr;
  word_t newAddr;

  if (arg1 == NULL || sscanf(arg1, "%u", &n) < 1){
    printf("we need two args and the first arg can't find\n");
    cmd_error_help("x");
    return 0;
  }
  printf("args:%s arg1:%s",args ,arg1);
  arg2 = args + strlen(arg1) + 1;
  if (arg2 >= str_end) {
    /*这说明没有第二个参数，这是错误的*/
    printf("we need two args and the second arg can't find\n");
    cmd_error_help("x");
    return 0;
  }
  exprAddr = expr(arg2, &success);
  if (success == false){
    printf("express is error\n");
    cmd_error_help("x");
    return 0;
  }
  for (i = 0; i < n; i++){
    newAddr = exprAddr + cnt * 32;
    if (cnt % 4 == 0){
      if (i != 0) printf("\n");
      printf("0x%-16x:", newAddr);
    }
    printf("0x%-16x", paddr_read(newAddr, 4));
    cnt++;
  }
  return 0;
}

static int cmd_p(char *args){

  return 0;
}