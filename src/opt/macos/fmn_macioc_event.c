#include "fmn_macioc_internal.h"

/* General window manager events.
 * Mostly not interesting.
 */

void fmn_macioc_cb_close(struct fmn_hw_video *video) {
  // macwm doesn't call this, closing the window always terminates the program.
  // I don't feel that's a problem, at least not enough to bother fixing.
  fmn_macioc_abort(0);
}

void fmn_macioc_cb_focus(struct fmn_hw_video *video,int focus) {
}

void fmn_macioc_cb_resize(struct fmn_hw_video *video,int w,int h) {
}

/* Wee event glue.
 */

static void fmn_macioc_toggle_fullscreen() {
  struct fmn_hw_video *video=fmn_hw_mgr_get_video(fmn_macioc.hw);
  if (!video) return;
  fmn_hw_video_set_fullscreen(video,-1);
}

/* Keyboard input.
 */
 
static struct fmn_macioc_keymap {
  int keycode;
  uint16_t btnid;
} fmn_macioc_keymapv[]={
// Must sort manually by (keycode).
  {0x00070004,FMN_BUTTON_LEFT},//a
  {0x00070007,FMN_BUTTON_RIGHT},//d
  {0x00070016,FMN_BUTTON_DOWN},//s
  {0x0007001a,FMN_BUTTON_UP},//w
  {0x0007001b,FMN_BUTTON_B},//x
  {0x0007001d,FMN_BUTTON_A},//z
  {0x00070036,FMN_BUTTON_B},//comma
  {0x00070037,FMN_BUTTON_A},//dot
  {0x0007004f,FMN_BUTTON_RIGHT},//right arrow
  {0x00070050,FMN_BUTTON_LEFT},//left arrow
  {0x00070051,FMN_BUTTON_DOWN},//down arrow
  {0x00070052,FMN_BUTTON_UP},//up arrow
  {0x00070058,FMN_BUTTON_B},//kpenter
  {0x0007005a,FMN_BUTTON_DOWN},//kp2
  {0x0007005c,FMN_BUTTON_LEFT},//kp4
  {0x0007005d,FMN_BUTTON_DOWN},//kp5
  {0x0007005e,FMN_BUTTON_RIGHT},//kp6
  {0x00070060,FMN_BUTTON_UP},//kp8
  {0x00070062,FMN_BUTTON_A},//kp0
};

int fmn_macioc_cb_key(struct fmn_hw_video *video,int keycode,int value) {
  //fprintf(stderr,"%s 0x%08x=%d\n",__func__,keycode,value);

  // Look for special stateless keys.
  if (value==1) switch (keycode) {
    case 0x00070029: fmn_macioc_abort(0); return 1; // ESC
    case 0x00070044: fmn_macioc_toggle_fullscreen(); return 1; // F11
  }
  
  if (value==2) return 0; // key-repeat is not interesting.
  
  int lo=0,hi=sizeof(fmn_macioc_keymapv)/sizeof(struct fmn_macioc_keymap),p=-1;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (keycode<fmn_macioc_keymapv[ck].keycode) hi=ck;
    else if (keycode>fmn_macioc_keymapv[ck].keycode) lo=ck+1;
    else { p=ck; break; }
  }
  if (p<0) return 0; // not a mapped key
  uint16_t btnid=fmn_macioc_keymapv[p].btnid;
  
  if (value) {
    if (fmn_macioc.input&btnid) return 1;
    fmn_macioc.input|=btnid;
  } else {
    if (!(fmn_macioc.input&btnid)) return 1;
    fmn_macioc.input&=~btnid;
  }
  return 1;
}

/* Text input. Probably not going to use.
 */

void fmn_macioc_cb_text(struct fmn_hw_video *video,int codepoint) {
  //fprintf(stderr,"%s U+%x\n",__func__,codepoint);
}

/* Mouse input. Definitely not going to use. Why did I write all this?
 */

void fmn_macioc_cb_mmotion(struct fmn_hw_video *video,int x,int y) {
}

void fmn_macioc_cb_mbutton(struct fmn_hw_video *video,int btnid,int value) {
}

void fmn_macioc_cb_mwheel(struct fmn_hw_video *video,int dx,int dy) {
}

/* Joystick events.
 */

#if FMN_USE_inmgr

void fmn_macioc_cb_connect(struct fmn_hw_input *input,int devid) {
  fmn_inmgr_connect(fmn_macioc.inmgr,input,devid);
}

void fmn_macioc_cb_disconnect(struct fmn_hw_input *input,int devid) {
  fmn_inmgr_disconnect(fmn_macioc.inmgr,devid);
}

void fmn_macioc_cb_event(struct fmn_hw_input *input,int devid,int btnid,int value) {
  fmn_inmgr_button(fmn_macioc.inmgr,devid,btnid,value);
}

void fmn_macioc_cb_premapped_event(struct fmn_hw_input *input,int devid,uint8_t btnid,int value) {
  fmn_inmgr_premapped(fmn_macioc.inmgr,devid,btnid,value);
}

#else

// Without inmap, we won't bother using generic input.

void fmn_macioc_cb_connect(struct fmn_hw_input *input,int devid) {}
void fmn_macioc_cb_disconnect(struct fmn_hw_input *input,int devid) {}
void fmn_macioc_cb_event(struct fmn_hw_input *input,int devid,int btnid,int value) {}

void fmn_macioc_cb_premapped_event(struct fmn_hw_input *input,int devid,uint8_t btnid,int value) {
  if (value) fmn_macioc.input|=btnid;
  else fmn_macioc.input&=~btnid;
}

#endif

void fmn_macioc_cb_state(struct fmn_inmgr *inmgr,uint8_t btnid,int value,uint8_t state) {
  if (value) fmn_macioc.input|=btnid;
  else fmn_macioc.input&=~btnid;
}

void fmn_macioc_cb_action(struct fmn_inmgr *inmgr,int btnid) {
  switch (btnid) {
    case FMN_INMGR_ACTION_QUIT: fmn_macioc_abort(0); break;
    case FMN_INMGR_ACTION_FULLSCREEN: fmn_macioc_toggle_fullscreen(); break;
  }
}
