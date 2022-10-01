#include "fmn_genioc_internal.h"

/* Close window.
 */
 
void fmn_genioc_close(struct fmn_hw_video *video) {
  fmn_genioc.terminate=1;
}

/* Resize window.
 */
 
void fmn_genioc_resize(struct fmn_hw_video *video,int w,int h) {
}

/* Input focus.
 */
 
void fmn_genioc_focus(struct fmn_hw_video *video,int focus) {
}

/* Raw keyboard input.
 */
 
int fmn_genioc_key(struct fmn_hw_video *video,int keycode,int value) {
  return 0;
}

/* Text input.
 */
 
void fmn_genioc_text(struct fmn_hw_video *video,int codepoint) {
}

/* Mouse events.
 */
 
void fmn_genioc_mmotion(struct fmn_hw_video *video,int x,int y) {
}

void fmn_genioc_mbutton(struct fmn_hw_video *video,int btnid,int value) {
}

void fmn_genioc_mwheel(struct fmn_hw_video *video,int dx,int dy) {
}

/* Generic input events.
 */
 
void fmn_genioc_connect(struct fmn_hw_input *input,int devid) {
}

void fmn_genioc_disconnect(struct fmn_hw_input *input,int devid) {
}

void fmn_genioc_button(struct fmn_hw_input *input,int devid,int btnid,int value) {
}

void fmn_genioc_premapped(struct fmn_hw_input *input,int devid,uint8_t btnid,int value) {
  if (value) fmn_genioc.instate|=btnid;
  else fmn_genioc.instate&=~btnid;
}
