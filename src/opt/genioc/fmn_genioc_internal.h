#ifndef FMN_GENIOC_INTERNAL_H
#define FMN_GENIOC_INTERNAL_H

#include "api/fmn_platform.h"
#include "api/fmn_client.h"
#include "opt/hw/fmn_hw.h"
#include "opt/time/fmn_time.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FMN_GENIOC_FRAME_RATE 60

extern struct fmn_genioc {

  struct fmn_hw_mgr *mgr;
  
  int terminate;
  volatile int sigc;
  
  double starttime;
  double starttimecpu;
  int framec;
  double nexttime;
  double frametime;
  int clockfaultc;
  
  uint8_t instate;
  
  // Keep the most frequently needed platform hook stuff readily available.
  struct fmn_hw_render *render;
  int (*render_begin)(struct fmn_hw_render *render);
  struct fmn_image *(*render_end)(struct fmn_hw_render *render);
  void (*render_fill_rect)(struct fmn_hw_render *render,int x,int y,int w,int h,uint32_t rgba);
  void (*render_blit)(
    struct fmn_hw_render *render,int dstx,int dsty,
    uint16_t imageid,int srcx,int srcy,
    int w,int h,
    uint8_t xform
  );
  void (*render_blit_tile)(struct fmn_hw_render *render,int dstx,int dsty,uint16_t imageid,uint8_t tileid,uint8_t xform);
  struct fmn_hw_video *video;
  void (*video_swap)(struct fmn_hw_video *video,struct fmn_image *fb);
  struct fmn_hw_audio *audio;
  int (*audio_lock)(struct fmn_hw_audio *audio);
  int (*audio_unlock)(struct fmn_hw_audio *audio);
  
} fmn_genioc;

void fmn_genioc_close(struct fmn_hw_video *video);
void fmn_genioc_resize(struct fmn_hw_video *video,int w,int h);
void fmn_genioc_focus(struct fmn_hw_video *video,int focus);
int fmn_genioc_key(struct fmn_hw_video *video,int keycode,int value);
void fmn_genioc_text(struct fmn_hw_video *video,int codepoint);
void fmn_genioc_mmotion(struct fmn_hw_video *video,int x,int y);
void fmn_genioc_mbutton(struct fmn_hw_video *video,int btnid,int value);
void fmn_genioc_mwheel(struct fmn_hw_video *video,int dx,int dy);
void fmn_genioc_connect(struct fmn_hw_input *input,int devid);
void fmn_genioc_disconnect(struct fmn_hw_input *input,int devid);
void fmn_genioc_button(struct fmn_hw_input *input,int devid,int btnid,int value);
void fmn_genioc_premapped(struct fmn_hw_input *input,int devid,uint8_t btnid,int value);

#endif
