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

#include <common.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
word_t expr(char *e, bool *success);

int main(int argc, char *argv[]) {
  FILE *file;
  char line[65536 + 128];
  char exprbuf[65536];
  uint32_t result;
  file = fopen("/home/cilinmengye/ics2023/nemu/tools/gen-expr/build/input", "r");
  assert(file != NULL);
  while (fgets(line, 65536 + 128, file) != NULL){
    int cnt = sscanf(line, "%u %s", &result, exprbuf);
    printf("%u %s\n", result, exprbuf);
    assert(cnt == 2);
    bool success = true;
    word_t ans = expr(exprbuf, &success);
    if (success == false || result - ans != 0){
      printf("expr: %s\n result: %u\n ans: %u\n", exprbuf, result, ans);
      return 0;
    }
  }
  return 0;
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
