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

#include "sdb.h"
#include "utils.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  word_t oldValue;
  char *express;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
/*这两个函数会作为监视点池的接口被其它函数调用*/
/*
 * new_wp()从free_链表中返回一个空闲的监视点结构
 * 调用new_wp()时可能会出现没有空闲监视点结构的情况, 为了简单起见, 此时可以通过assert(0)马上终止程序
 */
WP* new_wp(){
  Assert(free_ != NULL, "There is no free monitoring point returned in the free_ linked list");
  /*free monitoring point from free_*/
  WP* freeWP = free_;
  free_ = free_->next;
  /*link free monitoring point into head*/
  freeWP->next = head;
  head = freeWP;
  return head;
}
/*
 * free_wp()将wp归还到free_链表中
 */
void free_wp(WP *wp){
  Assert(head != NULL, "There is no busy monitoring point free into the head linked list");
  WP* front = head;
  WP* tail = NULL;
  bool success = false;
  
  while (front != NULL){
    if (front == wp){
      success = true;
      if (tail == NULL){
        /*indicate front = head*/
        Assert(front == head, "when tail == NULL, but front != NULL");
        head = head->next;
      } else {
        Assert(tail != NULL, "tail == NULL in free_wp");
        tail->next = front->next;
      }
      break;
    }
    tail = front;
    front = front->next;
  }
  /*过滤未找到的NO交给no2Wp函数*/
  Assert(success == true, "Can't find the corresponding wp in the head linked list");
  front->next = free_;
  free_ = front;
}

/*
 * 扫描所有的监视点
 * 在扫描监视点的过程中, 你需要对监视点的相应表达式进行求值(你之前已经实现表达式求值的功能了), 并比较它们的值有没有发生变化,
 * 若发生了变化, 程序就因触发了监视点而暂停下来, 你需要将nemu_state.state变量设置为NEMU_STOP来达到暂停的效果. 
 * 最后输出一句话提示用户触发了监视点, 并返回到sdb_mainloop()循环中等待用户的命令.
 */
void checkWatchPoint(){
  WP* freeWP = head;
  word_t newValue;
  bool success = true;

  while (freeWP != NULL){
    Assert(freeWP->express != NULL, "There are no express in watchpoint");
    newValue = expr(freeWP->express, &success);
    Assert(success == true, "The error express was put into watchpoint");
    if (newValue != freeWP->oldValue){
      nemu_state.state = NEMU_STOP;
      printf("Hardware watchpoint %d: %s\n\n", freeWP->NO, freeWP->express);
      printf("Old value = %u\n", freeWP->oldValue);
      printf("New value = %u\n", newValue);
    }
    freeWP = freeWP->next;
  }
}

void infoWatchPoint(){
  WP* freeWP = head;
  
  if (freeWP == NULL){
    printf("No watchpoints\n");
    return ;
  }
  printf("%-10s%s\n","Num","What");
  while (freeWP != NULL){
    printf("%-10d%s\n",freeWP->NO, freeWP->express);
    freeWP = freeWP->next;
  }
}

void free_wpByNO(int NO, bool *success){
  WP* freeWP = head;
  while (freeWP != NULL){
    if (freeWP->NO == NO){
      break;
    }
  }
  if (freeWP == NULL){
    *success = false;
    return;
  }
  free_wp(freeWP);
}

void new_wpSet(char *express, word_t oldValue){
  WP* freeWP;
  
  freeWP = new_wp();
  Assert(freeWP != NULL, "Error in cmd_w When call new_wp(), the return value is NULL");
  freeWP->express = express;
  freeWP->oldValue = oldValue;
}