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
#include <memory/paddr.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUMBER, TK_NEGATIVE, TK_HEX, TK_REG, TK_UNEQ, TK_AND, TK_POINTER

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
  {"\\(", '('},           // left parenthesis
  {"\\)", ')'},           // right parenthesis
  {"\\*", '*'},         // multiply
  {"/", '/'},           // division
  {"0x[0-9a-fA-F]+", TK_HEX},   // <hexadecimal-number>  
  {"(0u?|[1-9][0-9]*u?)", TK_NUMBER}, // decimal integer
  {"\\$[0-9a-zA-Z]+", TK_REG}, // <reg_name> 
  {"!=", TK_UNEQ},    // unequal
  {"&&", TK_AND},   // and 
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


//static Token tokens[32] __attribute__((used)) = {};
static Token tokens[65536] __attribute__((used)) = {};
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
          case TK_HEX:
          case TK_REG:
            //Assert(nr_token < 32, "The tokens array has insufficient storage space.");
            Assert(nr_token < 65536, "The tokens array has insufficient storage space.");
            Assert(substr_len < 32, "token is too long");
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;
          default:
            //Assert(nr_token < 32, "The tokens array has insufficient storage space.");
            Assert(nr_token < 65536, "The tokens array has insufficient storage space.");
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

word_t eval(int p, int q, bool *success);

void mark() {
  int i;

  /*markNegative*/
  for (i = 0; i < nr_token; i++){
    if (tokens[i].type == (int)('-') && (i == 0 || 
    (tokens[i - 1].type != TK_NUMBER && tokens[i - 1].type != (int)(')')))){
      tokens[i].type = TK_NEGATIVE;
    }
  }
  /*markPointer*/
  for (i = 0; i < nr_token; i++){
    if (tokens[i].type == (int)('*') && (i == 0 ||
    (tokens[i - 1].type != TK_NUMBER && tokens[i - 1].type != (int)(')')))){
      tokens[i].type = TK_POINTER;
    }
  }
}

void printfExpr(int p, int q){
  printf("expr:");
  for (int i = p; i <= q; i++){
    if (tokens[i].type == TK_NUMBER || tokens[i].type == TK_AND || 
        tokens[i].type == TK_POINTER || tokens[i].type == TK_HEX ||
        tokens[i].type == TK_REG || tokens[i].type == TK_NEGATIVE ||
        tokens[i].type == TK_UNEQ || tokens[i].type == TK_EQ){
        printf("%s", tokens[i].str);
    } else {
        printf("%c", (char)tokens[i].type);
    }
  }
  printf("\n");
}


word_t expr(char *e, bool *success) {

  bool evalSuccess;
  word_t exprAns;

  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
  mark();
  printfExpr(0, nr_token - 1);
  evalSuccess = true;
  exprAns = eval(0, nr_token - 1, &evalSuccess);
  *success = evalSuccess;
  if (evalSuccess){
    return exprAns;
  }
  return 0;
}

/*检查表达式的左右括号是否匹配*/
bool check_expr_parentheses(int p, int q){
  int i;
  int stack_top = -1;
  bool flag = true;

  for (i = p; i <= q ; i++){
    if (tokens[i].type == (int)('(')){
        stack_top++;
    } else if (tokens[i].type == (int)(')')){    
      if (stack_top >= 0){
        stack_top--;          
      } else {
        flag = false;
        break;
      }
    }
  }
  if (stack_top < 0 && flag == true){
    flag = true;
  } else {
    flag = false;
  }
  return flag;
}

bool check_parentheses(int p, int q){
    /*判断表达式是否被一对匹配的括号包围着*/
  if (tokens[p].type == (int)('(') && tokens[q].type == (int)(')')){
    return check_expr_parentheses(p + 1, q - 1);
  }
  return false;
}

int priority(int op_type){
    switch (op_type)
    {
    case '(':
    case ')':
        return 0;
    case TK_NEGATIVE:
    case TK_POINTER:
        return 1;
    case '+':
    case '-':
        return 2;
    case '*':
    case '/':
        return 3;
    case TK_EQ:
    case TK_UNEQ:
        return 4;
    case TK_AND:
        return 5;
    default:
        Assert(0, "No corresponding operator found");
    }
}

struct stack_node{
    int idx;
    int type;
};

word_t eval(int p, int q, bool *success){
  
  printfExpr(p, q);
  word_t number;

  if (p > q){
    *success = false;
    return 0;
  } else if (p == q){
    if (tokens[p].type == TK_HEX){
      if (sscanf(tokens[p].str,"%x", &number) < 1){
        *success = false;
        return 0;
      }
    } else if (tokens[p].type == TK_REG){
      /*tokens[p].str + 1 是为了去除前面的$*/
      number = isa_reg_str2val(tokens[p].str + 1, success);
      if (*success == false){
        return 0;
      }
    } else if (tokens[p].type == TK_NUMBER){
      if (sscanf(tokens[p].str,"%u", &number) < 1){
        *success = false;
        return 0;
      }
    }
    return number;
  } else if (check_parentheses(p, q) == true){
    return eval(p + 1, q - 1, success);
  } else {
    if (check_expr_parentheses(p, q) == false){
      *success = false;
      return 0;
    }
    int i;
    int top = -1;
    int op_type;
    word_t val1;
    word_t val2;
    /*这个stack数组不能设太大，否则会因为栈溢出导致段错误*/
    struct stack_node stack[1024];
    /*
     * 出现在一对括号中的token不是主运算符. 注意到这里不会出现有括号包围整个表达式的情况, 因为这种情况已经在check_parentheses()相应的if块中被处理了.
     * 主运算符的优先级在表达式中是最低的.
     * 当有多个运算符的优先级都是最低时, 根据结合性, 最后被结合的运算符才是主运算符
     * 一个例子是1 + 2 + 3, 它的主运算符应该是右边的+.
     * 有例外，比如负号和解指针结合性都是从右往左，如---1，那么应该取最左边的-
     */
    for (i = p; i <= q; i++){
        Assert(top < 1024, "stack in eval function over overflow!");
        /*去除非运算符*/
        if (tokens[i].type == TK_NUMBER || tokens[i].type == TK_HEX ||tokens[i].type == TK_REG) continue;
        if (top < 0){
            top++;
            stack[top].idx = i;
            stack[top].type = tokens[i].type;
            continue;
        }
        if (tokens[i].type == (int)('(')){
            top++;
            stack[top].idx = i;
            stack[top].type = tokens[i].type;
            continue;
        }
        if (tokens[i].type == (int)(')')){
            /*出现在一对括号中的token不是主运算符*/
            while (top >=0 && stack[top].type != '('){
                top--;
            }
            /*pop (*/
            top--;
            continue;
        }
        bool haveSub = false;
        /*优先级越低越在stack下面,主运算符的优先级在表达式中是最低的.当有多个运算符的优先级都是最低时, 根据结合性, 最后被结合的运算符才是主运算符*/
        while (top >= 0 && priority(tokens[i].type) <= priority(stack[top].type)){
            haveSub = false;
            if (tokens[i].type == stack[top].type && 
                  (stack[top].type == TK_NEGATIVE || 
                   stack[top].type == TK_POINTER)){
                break;
            }
            top--;
            haveSub = true;
        }
        if (haveSub){
          ++top;
          stack[top].idx = i;
          stack[top].type = tokens[i].type;
        }
    }

    Assert(top < 1024, "stack in eval function over overflow!");
    /*If the primary operator cannot be found, the expression is incorrect.*/
    if (top < 0){
      *success = false;
      return 0;
    }

    if (stack[0].type == TK_NEGATIVE || stack[0].type == TK_POINTER){
      val1 = -1;
      val2 = eval(stack[0].idx + 1, q, success);
    } else {
      val1 = eval(p, stack[0].idx - 1, success);
      val2 = eval(stack[0].idx + 1, q, success);
    }
    op_type = stack[0].type;

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
      if (val2 == 0){
        *success = false;
        return 0;
      }
      return val1 / val2;
    case TK_NEGATIVE:
      return val1 * val2;
    case TK_POINTER:
      return paddr_read(val2, 4);
    case TK_EQ:
      return val1 == val2;
    case TK_UNEQ:
      return val1 != val2;
    case TK_AND:
      return val1 && val2;  
    default:
      Assert(0, "No corresponding operator found");
    }
  }
}
