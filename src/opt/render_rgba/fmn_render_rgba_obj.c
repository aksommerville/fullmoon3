#include "fmn_render_rgba_internal.h"

/* Byte order.
 */
 
static inline uint32_t fmn_pixel_native_from_rgba(uint32_t rgba) {
  #if BYTE_ORDER==LITTLE_ENDIAN
    return (rgba>>24)|((rgba&0xff0000)>>8)|((rgba&0xff00)<<8)|(rgba<<24);
  #else
    return rgba;
  #endif
}

/* Cleanup.
 */
 
static void _rgba_del(struct fmn_hw_render *render) {
  fmn_image_del(RENDER->fb);
}

/* Init.
 */
 
static int _rgba_init(struct fmn_hw_render *render,const struct fmn_hw_render_params *params) {

  if (params) {
    render->fbw=params->fbw;
    render->fbh=params->fbh;
  }
  if (render->fbw<1) render->fbw=160;
  if (render->fbh<1) render->fbh=90;
  
  if (!(RENDER->fb=fmn_image_new_alloc(FMN_IMAGE_FMT_RGBA,render->fbw,render->fbh))) return -1;
  if (RENDER->fb->stride&3) return -1;
  
  return 0;
}

/* Upload image.
 */
 
static int _rgba_upload_image(struct fmn_hw_render *render,uint16_t imageid,struct fmn_image *image) {
  //TODO
  return 0;
}

/* Begin frame.
 */

static int _rgba_begin(struct fmn_hw_render *render) {
  return 0;
}

/* End frame.
 */
 
static struct fmn_image *_rgba_end(struct fmn_hw_render *render) {
  return RENDER->fb;
}

/* Fill rect.
 */
 
static void _rgba_fill_rect(struct fmn_hw_render *render,int x,int y,int w,int h,uint32_t rgba) {
  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x>RENDER->fb->w-w) w=RENDER->fb->w-x;
  if (y>RENDER->fb->h-h) h=RENDER->fb->h-y;
  if ((w<1)||(h<1)) return;
  rgba=fmn_pixel_native_from_rgba(rgba);
  int wstride=RENDER->fb->stride>>2;
  uint32_t *dstrow=((uint32_t*)RENDER->fb->v)+y*wstride+x;
  for (;h-->0;dstrow+=wstride) {
    uint32_t *dstp=dstrow;
    int xi=w;
    for (;xi-->0;dstp++) *dstp=rgba;
  }
}

/* Blit.
 */
 
static void _rgba_blit(
  struct fmn_hw_render *render,int dstx,int dsty,
  uint16_t imageid,int srcx,int srcy,
  int w,int h,
  uint8_t xform
) {
//TODO
}

/* Blit tile.
 */
 
static void _rgba_blit_tile(struct fmn_hw_render *render,int dstx,int dsty,uint16_t imageid,uint8_t tileid,uint8_t xform) {
//TODO
}

/* Type definition.
 */
 
const struct fmn_hw_render_type fmn_hw_render_type_rgba={
  .name="rgba",
  .desc="Software rendering in full color.",
  .objlen=sizeof(struct fmn_hw_render_rgba),
  .del=_rgba_del,
  .init=_rgba_init,
  .upload_image=_rgba_upload_image,
  .begin=_rgba_begin,
  .end=_rgba_end,
  .fill_rect=_rgba_fill_rect,
  .blit=_rgba_blit,
  .blit_tile=_rgba_blit_tile,
};
