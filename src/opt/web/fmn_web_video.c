#include "api/fmn_platform.h"
#include "game/image/fmn_image.h"

#define FMN_FBW 96
#define FMN_FBH 64

#define FMN_WEB_FB_STRIDE (FMN_FBW>>2)

static uint8_t fmn_web_fb_storage[FMN_WEB_FB_STRIDE*FMN_FBH];
static struct fmn_image fmn_web_fb={
  .v=fmn_web_fb_storage,
  .w=FMN_FBW,
  .h=FMN_FBH,
  .stride=FMN_WEB_FB_STRIDE,
  .fmt=FMN_IMAGE_FMT_Y2,
  .flags=FMN_IMAGE_FLAG_WRITEABLE,
};
static uint8_t fmn_web_fb_rgba[FMN_FBW*FMN_FBH*4];

void fmn_web_external_render(const void *v,int w,int h);

uint8_t fmn_platform_video_get_format() {
  return fmn_web_fb.fmt;
}

struct fmn_image *fmn_platform_video_begin() {
  return &fmn_web_fb;
}

static void fmn_web_cvtfb(uint8_t *dst,const uint8_t *src,int c) {
  int shift=6;
  for (;c-->0;dst+=4) {
    uint8_t y=((*src)>>shift)&3;
    y|=y<<2;
    y|=y<<4;
    dst[0]=dst[1]=dst[2]=y;
    dst[3]=0xff;
    if (shift) shift-=2;
    else { shift=6; src++; }
  }
}

void fmn_platform_video_end(struct fmn_image *fb) {
  if (fb!=&fmn_web_fb) return;
  fmn_web_cvtfb(fmn_web_fb_rgba,fb->v,FMN_FBW*FMN_FBH);
  fmn_web_external_render(fmn_web_fb_rgba,FMN_FBW,FMN_FBH);
}
