/* fmn_macwm_driver.c
 * Ideally this would be a thin pass-thru connecting macwm and Fullmoon's intf.
 * We must deal with the final scale-up tho; that was a completely separate concern where i copied macwm from.
 */

#include "fmn_macwm_internal.h"
#include <stdlib.h>

/* Instance definition.
 */

struct fmn_hw_video_macwm {
  struct fmn_hw_video hdr;
  struct fmn_image fb;
  uint8_t *fbcvt; // RGBA if not null (if null, assume provided framebuffer is RGBA)
};

#define DRIVER ((struct fmn_hw_video_macwm*)driver)

/* Cleanup.
 */

static void _macwm_del(struct fmn_hw_video *driver) {
  if (DRIVER->fbcvt) free(DRIVER->fbcvt);
  fmn_macwm_quit();
}

/* Examine requested framebuffer format, prep our conversion buffer if needed.
 */

static int macwm_init_framebuffer(struct fmn_hw_video *driver) {
  if (driver->fbw<1) return -1;
  if (driver->fbh<1) return -1;
  DRIVER->fb.w=driver->fbw;
  DRIVER->fb.h=driver->fbh;
  DRIVER->fb.fmt=driver->fbfmt;
  DRIVER->fb.flags=FMN_IMAGE_FLAG_WRITEABLE;
  switch (driver->fbfmt) {
    case FMN_IMAGE_FMT_RGBA: {
        DRIVER->fb.stride=driver->fbw<<2;
      } break;
    case FMN_IMAGE_FMT_Y8: {
        DRIVER->fb.stride=driver->fbw<<2;
        if (!(DRIVER->fbcvt=malloc(driver->fbw*driver->fbh*4))) return -1;
      } break;
    case FMN_IMAGE_FMT_Y2: {
        DRIVER->fb.stride=(driver->fbw+3)>>2;
        if (!(DRIVER->fbcvt=malloc(driver->fbw*driver->fbh*4))) return -1;
      } break;
    default: return -1;
  }
  if (!(DRIVER->fb.v=malloc(DRIVER->fb.stride*DRIVER->fb.h))) return -1;
  return 0;
}

/* Init.
 */

static int _macwm_init(struct fmn_hw_video *driver,const struct fmn_hw_video_params *params) {

  if (driver->fbw<1) {
    driver->fbw=96;
    driver->fbh=64;
    driver->fbfmt=FMN_IMAGE_FMT_Y2;
  }

  if (fmn_macwm_init(
    driver->fbw,driver->fbh,
    0/*driver->fullscreen*/,
    params?params->title:0,
    driver->delegate,driver
  )<0) return -1;

  if (macwm_init_framebuffer(driver)<0) {
    fprintf(stderr,
      "%s: Failed to initialize framebuffer for format %d, size %d,%d.\n",
      __func__,driver->fbfmt,driver->fbw,driver->fbh
    );
    return -1;
  }

  return 0;
}

/* RGBA from Y8
 */

static const void *fmn_macwm_cvt_rgba_y8(struct fmn_hw_video *driver,const uint8_t *srcrow) {
  int srcstride=driver->fbw;
  int dststride=driver->fbw<<2;
  uint8_t *dstrow=DRIVER->fbcvt;
  int yi=driver->fbh;
  for (;yi-->0;srcrow+=srcstride,dstrow+=dststride) {
    const uint8_t *srcp=srcrow;
    uint8_t *dstp=dstrow;
    int xi=driver->fbw;
    for (;xi-->0;srcp+=1,dstp+=4) {
      dstp[0]=*srcp;
      dstp[1]=*srcp;
      dstp[2]=*srcp;
      dstp[3]=0xff;
    }
  }
  return DRIVER->fbcvt;
}

/* RGBA from Y2
 */

static const void *fmn_macwm_cvt_rgba_y2(struct fmn_hw_video *driver,const uint8_t *srcrow) {
  int srcstride=(driver->fbw+3)>>2;
  int dststride=driver->fbw<<2;
  uint8_t *dstrow=DRIVER->fbcvt;
  int yi=driver->fbh;
  for (;yi-->0;srcrow+=srcstride,dstrow+=dststride) {
    int srcshift=6;
    const uint8_t *srcp=srcrow;
    uint8_t *dstp=dstrow;
    int xi=driver->fbw;
    for (;xi-->0;dstp+=4) {
      uint8_t luma=((*srcp)>>srcshift)&3;
      luma|=luma<<2;
      luma|=luma<<4;
      dstp[0]=luma;
      dstp[1]=luma;
      dstp[2]=luma;
      dstp[3]=0xff;
      if (srcshift) srcshift-=2;
      else { srcshift=6; srcp++; }
    }
  }
  return DRIVER->fbcvt;
}

/* Convert and upload texture.
 */

static int fmn_macwm_upload_texture(struct fmn_hw_video *driver,const void *fb) {
  const void *glpixels=0;
  if (DRIVER->fbcvt) switch (driver->fbfmt) {
    case FMN_IMAGE_FMT_Y8: glpixels=fmn_macwm_cvt_rgba_y8(driver,fb); break;
    case FMN_IMAGE_FMT_Y2: glpixels=fmn_macwm_cvt_rgba_y2(driver,fb); break;
  } else {
    glpixels=fb;
  }
  if (!glpixels) return -1;
  fmn_macwm_replace_fb(glpixels);
  return 0;
}

/* Swap.
 */

static struct fmn_image *_macwm_begin(struct fmn_hw_video *driver) {
  return &DRIVER->fb;
}

static void _macwm_end(struct fmn_hw_video *driver,struct fmn_image *fb) {
  if (fb!=&DRIVER->fb) return;
  fmn_macwm_upload_texture(driver,fb->v);
}

/* Fullscreen.
 */

static void _macwm_fullscreen(struct fmn_hw_video *driver,int state) {
  if (state>0) {
    if (driver->fullscreen) return;
    if (fmn_macwm_toggle_fullscreen()<0) return;
    driver->fullscreen=1;
  } else if (!state) {
    if (!driver->fullscreen) return;
    if (fmn_macwm_toggle_fullscreen()<0) return;
    driver->fullscreen=0;
  } else {
    if (fmn_macwm_toggle_fullscreen()<0) return;
  }
}

/* Type definition.
 */

const struct fmn_hw_video_type fmn_hw_video_type_macwm={
  .name="macwm",
  .desc="MacOS window manager",
  .objlen=sizeof(struct fmn_hw_video_macwm),
  .provides_system_keyboard=1,
  .del=_macwm_del,
  .init=_macwm_init,
  .begin=_macwm_begin,
  .end=_macwm_end,
  .set_fullscreen=_macwm_fullscreen,
};
