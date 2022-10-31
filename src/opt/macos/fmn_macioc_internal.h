#ifndef FMN_MACIOC_INTERNAL_H
#define FMN_MACIOC_INTERNAL_H

#include "api/fmn_platform.h"
#include "api/fmn_client.h"
#include "opt/hw/fmn_hw.h"
#include "game/image/fmn_image.h"
#include <string.h>

#if FMN_USE_inmgr
  #include "opt/inmgr/fmn_inmgr.h"
#endif

#ifdef __OBJC__
#include <Cocoa/Cocoa.h>
@interface AKAppDelegate : NSObject <NSApplicationDelegate> {
}
@end
#endif

#define FMN_MACIOC_FRAME_RATE 60
#define FMN_MACIOC_SLEEP_LIMIT 100000 /* us */

extern struct fmn_macioc {
  int init;
  int terminate;
  int update_in_progress;
  struct fmn_hw_mgr *hw;
  uint8_t input;
  int argc; // stashed here so we can examine in a more nested context (fmn_macioc_init)
  char **argv;
  
  double starttime;
  double starttimecpu;
  int framec;
  int64_t frametime;
  int64_t nexttime;

  #if FMN_USE_inmgr
    struct fmn_inmgr *inmgr;
  #endif
} fmn_macioc;

void fmn_macioc_abort(const char *fmt,...);
int fmn_macioc_call_init();
void fmn_macioc_call_quit();

void fmn_macioc_cb_close(struct fmn_hw_video *driver);
void fmn_macioc_cb_focus(struct fmn_hw_video *driver,int focus);
void fmn_macioc_cb_resize(struct fmn_hw_video *driver,int w,int h);
int fmn_macioc_cb_key(struct fmn_hw_video *driver,int keycode,int value);
void fmn_macioc_cb_text(struct fmn_hw_video *driver,int codepoint);
void fmn_macioc_cb_mmotion(struct fmn_hw_video *driver,int x,int y);
void fmn_macioc_cb_mbutton(struct fmn_hw_video *driver,int btnid,int value);
void fmn_macioc_cb_mwheel(struct fmn_hw_video *driver,int dx,int dy);
  
void fmn_macioc_cb_connect(struct fmn_hw_input *driver,int devid);
void fmn_macioc_cb_disconnect(struct fmn_hw_input *driver,int devid);
void fmn_macioc_cb_event(struct fmn_hw_input *driver,int devid,int btnid,int value);
void fmn_macioc_cb_premapped_event(struct fmn_hw_input *driver,int devid,uint8_t btnid,int value);

void fmn_macioc_cb_state(struct fmn_inmgr *inmgr,uint8_t btnid,int value,uint8_t state);
void fmn_macioc_cb_action(struct fmn_inmgr *inmgr,int btnid);

double fmn_macioc_now_s();
double fmn_macioc_now_cpu_s();
int64_t fmn_macioc_now_us();
void fmn_macioc_sleep_us(int us);

#endif
