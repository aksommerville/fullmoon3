#include "fmn_hw.h"
#include "game/image/fmn_image.h"

struct fmn_hw_video_nullvideo {
  struct fmn_hw_video hdr;
  struct fmn_image *fb;
};

#define VIDEO ((struct fmn_hw_video_nullvideo*)video)

static void _nullvideo_del(struct fmn_hw_video *video) {
  fmn_image_del(VIDEO->fb);
}

static int _nullvideo_init(struct fmn_hw_video *video,const struct fmn_hw_video_params *params) {
 
  video->fbw=params->fbw;
  video->fbh=params->fbh;
  if ((video->fbw<1)||(video->fbh<1)) {
    video->fbw=160;
    video->fbh=90;
  }
  video->winw=video->fbw;
  video->winh=video->fbh;
  video->fullscreen=1;
  
  if (!(VIDEO->fb=fmn_image_new_alloc(FMN_IMAGE_FMT_RGBA,video->fbw,video->fbh))) return -1;
  
  return 0;
}

static struct fmn_image *_nullvideo_begin(struct fmn_hw_video *video) {
  return VIDEO->fb;
}

static void _nullvideo_end(struct fmn_hw_video *video,struct fmn_image *fb) {
  if (fb!=VIDEO->fb) return;
  // Opportunity to take screencaps or whatever.
}

const struct fmn_hw_video_type fmn_hw_video_type_nullvideo={
  .name="nullvideo",
  .desc="Dummy video driver that does nothing. eg for automation.",
  .by_request_only=1,
  .objlen=sizeof(struct fmn_hw_video_nullvideo),
  .del=_nullvideo_del,
  .init=_nullvideo_init,
  .begin=_nullvideo_begin,
  .end=_nullvideo_end,
};
