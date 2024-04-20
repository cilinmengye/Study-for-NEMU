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
#include <device/map.h>
#include <SDL2/SDL.h>

#define INIT_OFFSET 16
/*
 * 声卡不能独立播放音频, 它需要接受来自客户程序的设置和音频数据. 程序要和设备交互
 * 我们需要定义一些寄存器和 MMIO空间来让程序访问
 * 
 * freq, channels和samples这三个寄存器可写入相应的初始化参数
 * init寄存器用于初始化, 写入后将根据设置好的freq, channels和samples来对SDL的音频子系统进行初始化
 * sbuf_size寄存器可读出流缓冲区的大小
 * count寄存器可以读出当前流缓冲区已经使用的大小
 */
enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;

/*
 * 通过SDL_OpenAudio()来初始化音频子系统, 需要提供频率, 格式等参数, 还需要注册一个用于将来填充音频数据的回调函数 
 * SDL库会定期调用初始化时注册的回调函数, 并提供一个缓冲区, 请求回调函数往缓冲区中写入音频数据
 * 回调函数返回后, SDL库就会按照初始化时提供的参数来播放缓冲区中的音频数据
 * 
 * 维护流缓冲区. 我们可以把流缓冲区可以看成是一个队列
 * 如果回调函数需要的数据量大于当前流缓冲区中的数据量, 你还需要把SDL提供的缓冲区剩余的部分清零, 
 */
static void audio_callback(void *userdata, Uint8 *stream, int len){
  int count = audio_base[5];
  if (count > len){
    SDL_memcpy(stream, sbuf, len);
    audio_base[5] = count - len;
    /*去除掉以及拷贝到SDL缓冲区的内容*/
    for (int i = 0; i < count; i++)
      sbuf[i] = sbuf[len + i];
  } else {
    SDL_memcpy(stream, sbuf, count);
    SDL_memset(stream + audio_base[5], 0, len - count);
    audio_base[5] = 0;
  }
}

static void audio_init(){
  SDL_AudioSpec s = {}; 
  s.format = AUDIO_S16SYS;  // 假设系统中音频数据的格式总是使用16位有符号数来表示
  s.userdata = NULL;        // 不使用
  s.freq = audio_base[0]; // 采样频率
  s.channels = audio_base[1]; // 声道数
  s.samples = audio_base[2]; // 缓冲区大小
  s.callback = audio_callback; // 回调函数

  // if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
  //   panic("Failed to initialize SDL audio: %s\n", SDL_GetError());
  // if (SDL_OpenAudio(&s, NULL) < 0) 
  //   panic("Failed to open audio device: %s\n", SDL_GetError());
  SDL_InitSubSystem(SDL_INIT_AUDIO);
  SDL_OpenAudio(&s, NULL);
  SDL_PauseAudio(0);
}


static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  assert(offset == 0 || offset == 4 || offset == 8 || offset == 12 ||
         offset == 16 || offset == 20 || offset == 24);
  switch (offset)
  {
  case INIT_OFFSET:
    if (is_write && audio_base[4]) audio_init();
    break; 
  default:
    break;
  }
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
  audio_base[3] = CONFIG_SB_SIZE;
  audio_base[5] = 0;
  audio_base[6] = 7;
/*
 * NEMU的简单声卡在初始化时会分别注册0x200处长度为24个字节的端口,
 * 以及0xa0000200处长度为24字节的MMIO空间, 它们都会映射到上述寄存器
 */
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif
  /*
   * 流缓冲区STREAM_BUF是一段MMIO空间, 用于存放来自程序的音频数据, 这些音频数据会在将来写入到SDL库中
   */
  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
