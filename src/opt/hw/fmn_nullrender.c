#include "fmn_hw.h"

/* Object definition.
 */
 
struct fmn_hw_render_nullrender {
  struct fmn_hw_render hdr;
};

#define RENDER ((struct fmn_hw_render_nullrender*)render)

/* Stubs.
 */
 
static void _nullrender_del(struct fmn_hw_render *render) {
}
 
static int _nullrender_init(struct fmn_hw_render *render,const struct fmn_hw_render_params *params) {

  if (params) {
    render->fbw=params->fbw;
    render->fbh=params->fbh;
  }
  if (render->fbw<1) render->fbw=160;
  if (render->fbh<1) render->fbh=90;
  
  return 0;
}
 
static int _nullrender_upload_image(struct fmn_hw_render *render,uint16_t imageid,struct fmn_image *image) {
  return 0;
}

static int _nullrender_begin(struct fmn_hw_render *render) {
  return 0;
}

static struct fmn_image *_nullrender_end(struct fmn_hw_render *render) {
  return 0;
}

static void _nullrender_fill_rect(struct fmn_hw_render *render,int x,int y,int w,int h,uint32_t rgba) {
}

static void _nullrender_blit(
  struct fmn_hw_render *render,int dstx,int dsty,
  uint16_t imageid,int srcx,int srcy,
  int w,int h,
  uint8_t xform
) {
}

static void _nullrender_blit_tile(struct fmn_hw_render *render,int dstx,int dsty,uint16_t imageid,uint8_t tileid,uint8_t xform) {
}

/* Type definition.
 */
 
const struct fmn_hw_render_type fmn_hw_render_type_nullrender={
  .name="nullrender",
  .desc="Dummy renderer that does nothing.",
  .by_request_only=1,
  .objlen=sizeof(struct fmn_hw_render_nullrender),
  .del=_nullrender_del,
  .init=_nullrender_init,
  .upload_image=_nullrender_upload_image,
  .begin=_nullrender_begin,
  .end=_nullrender_end,
  .fill_rect=_nullrender_fill_rect,
  .blit=_nullrender_blit,
  .blit_tile=_nullrender_blit_tile,
};
