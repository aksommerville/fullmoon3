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
 
uint8_t fmn_platform_video_get_format() {
  if (!fmn_genioc.video) return 0;
  return fmn_genioc.video->fbfmt;
}
 
struct fmn_image *fmn_platform_video_begin() {
  if (!fmn_genioc.video_begin) return 0;
  return fmn_genioc.video_begin(fmn_genioc.video);
}

void fmn_platform_video_end(struct fmn_image *fb) {
  if (!fmn_genioc.video_end) return;
  fmn_genioc.video_end(fmn_genioc.video,fb);
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
