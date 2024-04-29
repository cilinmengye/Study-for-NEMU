#include <NDL.h>
#include <SDL.h>
#include <assert.h>
#include <string.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

/*
 * SDL_PollEvent(): 它和SDL_WaitEvent()不同的是, 如果当前没有任何事件, 就会立即返回
 * Returns 1 if there is a pending event or 0 if there are none available.
 */
int SDL_PollEvent(SDL_Event *event) {
  char buf[64];
  if (NDL_PollEvent(buf, 64) == 0) return 0;
  else {
    char keytype[4];
    char keycode[32];
    sscanf(buf, "%s %s", keytype, keycode);
    if (strcmp(keytype, "kd") == 0) event->type = SDL_KEYDOWN;
    else if (strcmp(keytype, "ku") == 0) event->type = SDL_KEYUP;
    else assert(0);
    for (int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++){
      if (strcmp(keycode, keyname[i]) == 0){
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
        printf("SDL_WaitEvent ret: %d %s\n", ret, keycode);
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
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
