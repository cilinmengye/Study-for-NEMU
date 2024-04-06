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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUMBER

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", '-'},         // sub
  {"(", '('},           // left parenthesis
  {")", ')'},           // right parenthesis
  {"\\*", '*'},         // multiply
  {"/", '/'},           // division
  {"(0|[1-9][0-9]*)", TK_NUMBER}, // decimal integer
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_NUMBER:
            Assert(nr_token < 32, "The tokens array has insufficient storage space.");
            Assert(substr_len < 32, "token is too long");
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;
          default:
            Assert(nr_token < 32, "The tokens array has insufficient storage space.");
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p, int q);
word_t eval(int p, int q, bool *success);

word_t expr(char *e, bool *success) {
  bool evalSuccess;
  word_t exprAns;

  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();

  evalSuccess = true;
  exprAns = eval(0, nr_token-1, &evalSuccess);
  *success = evalSuccess;
  if (evalSuccess){
    return exprAns;
  }
  return 0;
}

bool check_parentheses(int p, int q){
  if (tokens[p].type == (int)('(') && tokens[q].type == (int)(')')){
    return true;
  }
  return false;
}

word_t eval(int p, int q, bool *success){
  if (p > q){
    *success = false;
    return 0;
  } else if (p == q){
    word_t number;

    if (sscanf(tokens[p].str,"%u", &number) < 1){
      *success = false;
      return 0;
    }
    return number;
  } else if (check_parentheses(p, q) == true){
    return eval(p+1, q-1, success);
  } else {
    int op;
    int op_type;
    bool isNegative = false;
    word_t val1;
    word_t val2;

    for (op = p; op <= q; op++){
      if (tokens[op].type != TK_NUMBER){
        break;
      }
    }
    /*If the primary operator cannot be found, the expression is incorrect.*/
    if (tokens[op].type == TK_NUMBER){
      *success = false;
      return 0;
    }
    if (tokens[op].type == (int)('-') && 
    ( op == p || (tokens[op - 1].type != TK_NUMBER && tokens[op - 1].type != (int)(')')) )
    ){
      /*Decide that he is a negative sign, not a sub sign*/
      isNegative = true;
      op_type = '*';
    } else {
      op_type = tokens[op].type;
    }

    if (isNegative){
      val1 = -1;
      val2 = eval(op + 1, q, success);
    } else {
      val1 = eval(p, op - 1, success);
      val2 = eval(op + 1, q, success);
    }

    /*Something went wrong in a step of the recursive solution.*/
    if (*success != true){
      return 0; 
    }
    switch (op_type)
    {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      Assert(val2 != 0, "Disable divide-by-zero operations");
      return val1 / val2;
    default:
      Assert(0, "No corresponding operator found");
    }
  }
}
