#include "fmn_macwm_internal.h"

struct fmn_macwm fmn_macwm={0};

/* Init.
 */

int fmn_macwm_init(
  int w,int h,
  int fullscreen,
  const char *title,
  const struct fmn_hw_delegate *delegate,
  struct fmn_hw_video *driver
) {
  if (fmn_macwm.window) {
    return -1;
  }
  memset(&fmn_macwm,0,sizeof(struct fmn_macwm));

  if (!(fmn_macwm.window=[FmnWindow 
    newWithWidth:w height:h title:title fullscreen:fullscreen 
    hw_delegate:delegate driver:driver
  ])) {
    fprintf(stderr,"FmnWindow ctor error\n");
    return -1;
  }

  return 0;
}

/* Quit.
 */

void fmn_macwm_quit() {
  [fmn_macwm.window release];

  memset(&fmn_macwm,0,sizeof(struct fmn_macwm));
}

/* Abort.
 */
 
void fmn_macwm_abort(const char *fmt,...) {
  if (fmt&&fmt[0]) {
    va_list vargs;
    va_start(vargs,fmt);
    char msg[256];
    int msgc=vsnprintf(msg,sizeof(msg),fmt,vargs);
    if ((msgc<0)||(msgc>=sizeof(msg))) msgc=0;
    fprintf(stderr,"%.*s\n",msgc,msg);
  }
  [NSApplication.sharedApplication terminate:nil];
}

/* Test cursor within window, based on last reported position.
 */

int fmn_macwm_cursor_within_window() {
  if (!fmn_macwm.window) return 0;
  if (fmn_macwm.window->mousex<0) return 0;
  if (fmn_macwm.window->mousey<0) return 0;
  if (fmn_macwm.window->mousex>=fmn_macwm.window->w) return 0;
  if (fmn_macwm.window->mousey>=fmn_macwm.window->h) return 0;
  return 1;
}

/* Show or hide cursor.
 */

int fmn_macwm_show_cursor(int show) {
  if (!fmn_macwm.window) return -1;
  if (show) {
    if (fmn_macwm.window->cursor_visible) return 0;
    if (fmn_macwm_cursor_within_window()) {
      [NSCursor unhide];
    }
    fmn_macwm.window->cursor_visible=1;
  } else {
    if (!fmn_macwm.window->cursor_visible) return 0;
    if (fmn_macwm_cursor_within_window()) {
      [NSCursor hide];
    }
    fmn_macwm.window->cursor_visible=0;
  }
  return 0;
}

/* Toggle fullscreen.
 */

int fmn_macwm_toggle_fullscreen() {

  [fmn_macwm.window toggleFullScreen:fmn_macwm.window];

  // Take it on faith that the state will change:
  return fmn_macwm.window->fullscreen^1;
}

/* Ridiculous hack to ensure key-up events.
 * Unfortunately, during a fullscreen transition we do not receive keyUp events.
 * If the main input is a keyboard, and the user strikes a key to toggle fullscreen,
 * odds are very strong that they will release that key during the transition.
 * We record every key currently held, and forcibly release them after on a fullscreen transition.
 */
 
int fmn_macwm_record_key_down(int key) {
  int p=-1;
  int i=FMN_MACWM_KEYS_DOWN_LIMIT; while (i-->0) {
    if (fmn_macwm.keys_down[i]==key) return 0;
    if (!fmn_macwm.keys_down[i]) p=i;
  }
  if (p>=0) {
    fmn_macwm.keys_down[p]=key;
  }
  return 0;
}

int fmn_macwm_release_key_down(int key) {
  int i=FMN_MACWM_KEYS_DOWN_LIMIT; while (i-->0) {
    if (fmn_macwm.keys_down[i]==key) {
      fmn_macwm.keys_down[i]=0;
    }
  }
  return 0;
}

int fmn_macwm_drop_all_keys() {
  int i=FMN_MACWM_KEYS_DOWN_LIMIT; while (i-->0) {
    if (fmn_macwm.keys_down[i]) {
      int key=fmn_macwm_translate_keysym(fmn_macwm.keys_down[i]);
      if (key) {
        const struct fmn_hw_delegate *dl=&fmn_macwm.window->hw_delegate;
        if (dl->key) {
          if (dl->key(fmn_macwm.window->driver,key,0)<0) return -1;
        }
      }
      fmn_macwm.keys_down[i]=0;
    }
  }
  return 0;
}
