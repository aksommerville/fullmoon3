#include "fmn_genioc_internal.h"

/* State change from inmgr.
 */
 
void fmn_genioc_inmgr_state(struct fmn_inmgr *inmgr,uint8_t btnid,int value,uint8_t state) {
  fmn_genioc.instate=state;
}

void fmn_genioc_inmgr_action(struct fmn_inmgr *inmgr,int btnid) {
  switch (btnid) {
    case FMN_INMGR_ACTION_QUIT: fmn_genioc.terminate=1; break;
    case FMN_INMGR_ACTION_FULLSCREEN: if (fmn_genioc.video) {
        fmn_hw_video_set_fullscreen(fmn_genioc.video,fmn_genioc.video->fullscreen?0:1);
      } break;
  }
}

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
  if (value==2) return 1; // repeats are not interesting
  if (fmn_inmgr_button(fmn_genioc.inmgr,fmn_genioc.devid_keyboard,keycode,value)<0) return -1;
  return 1; // Ack all events; we actually never want text.
}

/* Text and mouse events -- we're not using these.
 */
 
void fmn_genioc_text(struct fmn_hw_video *video,int codepoint) {
}
 
void fmn_genioc_mmotion(struct fmn_hw_video *video,int x,int y) {
}

void fmn_genioc_mbutton(struct fmn_hw_video *video,int btnid,int value) {
}

void fmn_genioc_mwheel(struct fmn_hw_video *video,int dx,int dy) {
}

/* Generic input events.
 */
 
void fmn_genioc_connect(struct fmn_hw_input *input,int devid) {
  fmn_inmgr_connect(fmn_genioc.inmgr,input,devid);
}

void fmn_genioc_disconnect(struct fmn_hw_input *input,int devid) {
  fmn_inmgr_disconnect(fmn_genioc.inmgr,devid);
}

void fmn_genioc_button(struct fmn_hw_input *input,int devid,int btnid,int value) {
  fmn_inmgr_button(fmn_genioc.inmgr,devid,btnid,value);
}

void fmn_genioc_premapped(struct fmn_hw_input *input,int devid,uint8_t btnid,int value) {
  fmn_inmgr_premapped(fmn_genioc.inmgr,devid,btnid,value);
}
