#include <am.h>
#include <nemu.h>
#include <stdio.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)

/*init寄存器用于初始化, 写入后将根据设置好的freq, channels和samples来对SDL的音频子系统进行初始化*/
void __am_audio_init() {
  outl(AUDIO_INIT_ADDR, 1);
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = inl(AUDIO_SBUF_SIZE_ADDR); 
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
  __am_audio_init();
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

/*
 * AM_AUDIO_PLAY, AM声卡播放寄存器, 可将[buf.start, buf.end)区间的内容作为音频数据写入流缓冲区. 
 * 若当前流缓冲区的空闲空间少于即将写入的音频数据, 此次写入将会一直等待, 直到有足够的空闲空间将音频数据完全写入流缓冲区才会返回.
 * 
 * 维护流缓冲区. 我们可以把流缓冲区可以看成是一个队列, 程序通过AM_AUDIO_PLAY的抽象往流缓冲区里面写入音频数据,
 */
void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  int len = ctl->buf.end - ctl->buf.start;
  int bufsize = io_read(AM_AUDIO_CONFIG).bufsize;
  int remainlen =  bufsize - io_read(AM_AUDIO_STATUS).count;
  while (remainlen < len){
    remainlen = bufsize - io_read(AM_AUDIO_STATUS).count;
  }
  uint32_t sbufAddr = AUDIO_SBUF_ADDR + io_read(AM_AUDIO_STATUS).count;
  uint32_t freq = io_read(AM_AUDIO_CTRL).freq;
  unsigned long last = 0;
  for (int i = 0; i < len; i++){
    unsigned long upt = io_read(AM_TIMER_UPTIME).us / 1000;
    while ((upt - last)*freq < 1000){
      upt = io_read(AM_TIMER_UPTIME).us / 1000;
    }
    outb(sbufAddr, *(uint8_t *)(ctl->buf.start + i));
    outl(AUDIO_COUNT_ADDR, io_read(AM_AUDIO_STATUS).count + 1);
    sbufAddr = AUDIO_SBUF_ADDR + io_read(AM_AUDIO_STATUS).count;
    last = upt;
  }
}
