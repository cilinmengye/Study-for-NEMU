#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

/*
 * 每当用户敲下/释放按键时, 将会把相应的键盘码放入数据寄存器, CPU可以访问数据寄存器, 获得键盘码; 
 * 当无按键可获取时, 将会返回AM_KEY_NONE.
 */
// void send_key(uint8_t scancode, bool is_keydown) {
//   if (nemu_state.state == NEMU_RUNNING && keymap[scancode] != NEMU_KEY_NONE) {
//     uint32_t am_scancode = keymap[scancode] | (is_keydown ? KEYDOWN_MASK : 0);
//     key_enqueue(am_scancode);
//   }
// }
void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t am_scancode = inl(KBD_ADDR);
  kbd->keydown = am_scancode & 0x8000;
  kbd->keycode = am_scancode & 0xff;
}
