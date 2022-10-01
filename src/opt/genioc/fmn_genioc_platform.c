#include "fmn_genioc_internal.h"

/* Init.
 * Ignore this; we do all the init before calling setup().
 */
 
int8_t fmn_platform_init() {
  return 0;
}

/* Terminate.
 */

void fmn_platform_terminate(uint8_t status) {
  fmn_genioc.terminate=1;
}

/* Update.
 * We pump the drivers in main().
 * Here, all we need to do is return the gathered input state.
 */

uint8_t fmn_platform_update() {
  return fmn_genioc.instate;
}

/* Driver calls.
 * Platform and driver APIs are uncoincidentally similar.
 */
 
void fmn_platform_video_upload_image(
  uint16_t imageid,
  struct fmn_image *image
) {
  if (!fmn_genioc.render) return;
  fmn_hw_render_upload_image(fmn_genioc.render,imageid,image);
}
 
void fmn_platform_video_begin() {
  if (!fmn_genioc.render_begin) return;
  fmn_genioc.render_begin(fmn_genioc.render);
}

void fmn_platform_video_end() {
  struct fmn_image *fb=0;
  if (fmn_genioc.render_end) fb=fmn_genioc.render_end(fmn_genioc.render);
  if (fmn_genioc.video_swap) fmn_genioc.video_swap(fmn_genioc.video,fb);
}

void fmn_platform_video_fill_rect(
  int16_t x,int16_t y,int16_t w,int16_t h,
  uint32_t rgba
) {
  if (!fmn_genioc.render_fill_rect) return;
  fmn_genioc.render_fill_rect(fmn_genioc.render,x,y,w,h,rgba);
}

void fmn_platform_video_blit(
  int16_t dstx,int16_t dsty,
  uint16_t srcimageid,
  int16_t srcx,int16_t srcy,
  int16_t w,int16_t h,
  uint8_t xform
) {
  if (!fmn_genioc.render_blit) return;
  fmn_genioc.render_blit(fmn_genioc.render,dstx,dsty,srcimageid,srcx,srcy,w,h,xform);
}

void fmn_platform_video_blit_tile(
  int16_t dstx,int16_t dsty,
  uint16_t srcimageid,
  uint8_t tileid,
  uint8_t xform
) {
  if (!fmn_genioc.render_blit_tile) return;
  fmn_genioc.render_blit_tile(fmn_genioc.render,dstx,dsty,srcimageid,tileid,xform);
}

void fmn_platform_audio_configure(const void *v,uint16_t c) {
  struct fmn_hw_synth *synth=fmn_hw_mgr_get_synth(fmn_genioc.mgr);
  if (!synth) return;
  if (fmn_genioc.audio_lock) fmn_genioc.audio_lock(fmn_genioc.audio);
  fmn_hw_synth_configure(synth,v,c);
  if (fmn_genioc.audio_unlock) fmn_genioc.audio_unlock(fmn_genioc.audio);
}

void fmn_platform_audio_play_song(const void *v,uint16_t c) {
  struct fmn_hw_synth *synth=fmn_hw_mgr_get_synth(fmn_genioc.mgr);
  if (!synth) return;
  if (fmn_genioc.audio_lock) fmn_genioc.audio_lock(fmn_genioc.audio);
  fmn_hw_synth_play_song(synth,v,c);
  if (fmn_genioc.audio_unlock) fmn_genioc.audio_unlock(fmn_genioc.audio);
}

void fmn_platform_audio_note(uint8_t programid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms) {
  struct fmn_hw_synth *synth=fmn_hw_mgr_get_synth(fmn_genioc.mgr);
  if (!synth) return;
  if (fmn_genioc.audio_lock) fmn_genioc.audio_lock(fmn_genioc.audio);
  fmn_hw_synth_note(synth,programid,noteid,velocity,duration_ms);
  if (fmn_genioc.audio_unlock) fmn_genioc.audio_unlock(fmn_genioc.audio);
}

void fmn_platform_audio_silence() {
  struct fmn_hw_synth *synth=fmn_hw_mgr_get_synth(fmn_genioc.mgr);
  if (!synth) return;
  if (fmn_genioc.audio_lock) fmn_genioc.audio_lock(fmn_genioc.audio);
  fmn_hw_synth_silence(synth);
  if (fmn_genioc.audio_unlock) fmn_genioc.audio_unlock(fmn_genioc.audio);
}

void fmn_platform_audio_release_all() {
  struct fmn_hw_synth *synth=fmn_hw_mgr_get_synth(fmn_genioc.mgr);
  if (!synth) return;
  if (fmn_genioc.audio_lock) fmn_genioc.audio_lock(fmn_genioc.audio);
  fmn_hw_synth_release_all(synth);
  if (fmn_genioc.audio_unlock) fmn_genioc.audio_unlock(fmn_genioc.audio);
}
