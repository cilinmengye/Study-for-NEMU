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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";
static int buf_index = 0;

/*Generate a random number less than n*/
static uint32_t choose(uint32_t n){
  return (uint32_t)((double)rand() / ((double)RAND_MAX + 1) * n);
}

static void gen(char c){
  buf[buf_index++] = c;
  buf[buf_index] = '\0';
}

static void gen_num(){
  /*
   * the number of unsigned max is 4294967295
   * so, to simplify, gen_num function will generate a number with bits ranging between 0 and 9;
   */
  uint32_t len = choose(10);
  if (len == (uint32_t)(0))
    len += 1;
  if (len == 1){
    gen((char)(choose(10) + '0'));
    return ;
  }
  uint32_t firstNum = choose(10);
  /*Numbers cannot start with 0*/
  if (firstNum == (uint32_t)(0)){
    firstNum += 1;
  }
  gen((char)(firstNum + '0'));
  uint32_t i;
  for (i = 2; i <= len; i++){
    gen((char)(choose(10) + '0'));
  }
}

/*Randomly generate 0~10 spaces*/
static void gen_space(){
  uint32_t len = choose(11);
  uint32_t i;
  
  for (i = 1; i <= len; i++){
    gen(' ');
  }
}

static void gen_rand_op(){
  switch (choose(4))
  {
  case 0: gen('+'); break;
  case 1: gen('-'); break;
  case 2: gen('*'); break;
  default: gen('/'); break;
  }
}

static void gen_rand_expr(uint32_t n) {
  uint32_t chooseAns = choose(3);
  /*
   * Up to ten levels of recursion
   * Then let gen_rand_expr force no more recursion
   */
  if (n >= 10){
    chooseAns = 0;
  }
  switch (chooseAns)
  {
  case 0: gen_space(); gen_num(); gen('u'); gen_space(); break;
  case 1: gen('('); gen_space(); gen_rand_expr(n + 1); gen_space(); gen(')'); break;
  default: gen_rand_expr(n + 1); gen_space(); gen_rand_op(); gen_space(); gen_rand_expr(n + 1); break; 
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    /*Reset buf_index! This is really important unless you want to spend 2 hours debugging like me.*/
    buf_index = 0;
    gen_rand_expr(1);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -o /tmp/.expr -Wall -Werror /tmp/.code.c");
    if (ret != 0) continue;
    
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    int status;
    ret = fscanf(fp, "%d", &result);
    status = pclose(fp);
    //printf("WIFEXITED(status):%d ----- WEXITSTATUS(status):%d\n", WIFEXITED(status), WEXITSTATUS(status));
    if (WIFEXITED(status)){
      /*indicates a divide-by-zero operation*/
      if (WEXITSTATUS(status) == 136){
        continue;
      }
    }
    printf("%u %s\n", result, buf);
  }
  return 0;
}
