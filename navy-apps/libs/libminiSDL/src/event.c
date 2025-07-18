#include <NDL.h>
#include <SDL.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

// 一个包含 SDL 为所有键分配的槽个数 的数组，记录这些键的状态
static uint8_t* keyboard_state = NULL;
static int num_keyboard_keys = 0;

void SDL_InitKeyboard() {
  num_keyboard_keys = sizeof(keyname) / sizeof(keyname[0]);
  assert(keyboard_state == NULL);
  keyboard_state = (uint8_t *)malloc(num_keyboard_keys * sizeof(uint8_t));
  memset(keyboard_state, 0, num_keyboard_keys * sizeof(uint8_t));
}

void SDL_QuitKeyboard() {
  if ( keyboard_state ) {
    free(keyboard_state);
    keyboard_state = NULL;
    num_keyboard_keys = 0;
  }
}

int SDL_PushEvent(SDL_Event *ev) {
  assert(0);
  return 0;
}

/*
 * SDL_PollEvent(): 它和SDL_WaitEvent()不同的是, 如果当前没有任何事件, 就会立即返回
 * Returns 1 if there is a pending event or 0 if there are none available.
 * 
 * 在操作系统event_read函数中我们作出假设：
 * 我们可以假设一次最多只会读出一个事件, 这样可以简化你的实现, 这里我们依旧延续上述假设
 */
int SDL_PollEvent(SDL_Event *event) {
  assert(keyboard_state != NULL);
  memset(keyboard_state, 0, sizeof(uint8_t) * num_keyboard_keys);

  char buf[64];
  if (NDL_PollEvent(buf, 64) == 0) return 0;

  char keytype[4];
  char keycode[32];
  sscanf(buf, "%s %s", keytype, keycode);
  if (strcmp(keytype, "kd") == 0) event->type = SDL_KEYDOWN;
  else if (strcmp(keytype, "ku") == 0) event->type = SDL_KEYUP;
  else assert(0);

  for (int i = 0; i < num_keyboard_keys; i++){
    if (strcmp(keycode, keyname[i]) == 0){
      assert(i != 0);

      if(event->type == SDL_KEYDOWN) keyboard_state[i] = 1;
      event->key.type = event->type;
      event->key.keysym.sym = i;
      return 1;
    }
  }
  
  assert(0);
  return 0;
}

/*
 * 在miniSDL中实现SDL_WaitEvent(), 它用于等待一个事件. 你需要将NDL中提供的事件封装成SDL事件返回给应用程序
 * Returns 1 on success or 0 if there was an error while waiting for events
 */
int SDL_WaitEvent(SDL_Event *event) {
  char buf[64];
  int ret;
  while ((ret = NDL_PollEvent(buf, 64)) != 0){
    char keytype[4];
    char keycode[32];
    sscanf(buf, "%s %s", keytype, keycode);
    if (strcmp(keytype, "kd") == 0) event->type = SDL_KEYDOWN;
    else if (strcmp(keytype, "ku") == 0) event->type = SDL_KEYUP;
    else assert(0);
    for (int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++){
      if (strcmp(keycode, keyname[i]) == 0){
        //printf("SDL_WaitEvent ret: %d %s\n", ret, keycode);
        event->key.type = event->type;
        event->key.keysym.sym = i;
        assert(i != 0);
        return 1;
      }
    }
    assert(0); 
  }
  return 0;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  assert(0);
  return 0;
}

/*
 * 返回值:返回一个指向内部状态数组的指针。数组的每个元素都是一个 Uint8，其值为
 * - 1：对应位置的按键当前被按下
 * - 0：对应位置的按键当前未被按下
 * 参数 numkeys
 * 如果不为 NULL，函数会将数组的长度（也就是 SDL 为所有键分配的槽个数）写入 *numkeys。
 * 通常你只需要关心特定键的状态，所以传 NULL 即可。
 * 注意：这块内存是 SDL 内部维护的，不应由调用者 free()。
 * 
 * 在SDL源代码中全局按键状态数组指针定义在src/events/SDL_keyboard.c
 * 在第一次初始化键盘子系统的时候（SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) 内部会调用 InitKeyboard()）
 * 注册后续事件泵（SDL_PumpEvents）时要更新这块数组。
 * SDL_PollEvents函数会调用SDL_PumpEvents，所以这里简单起见，我们在SDL_PollEvents函数中更新数组
 */
uint8_t* SDL_GetKeyState(int *numkeys) {
    if (numkeys) {
        *numkeys = num_keyboard_keys;
    }
    return keyboard_state;
}