#include "fmn_genioc_internal.h"
#include <signal.h>

struct fmn_genioc fmn_genioc={0};

/* Cleanup.
 */
 
static void fmn_genioc_cleanup() {
  fmn_hw_mgr_del(fmn_genioc.mgr);
  fmn_inmgr_del(fmn_genioc.inmgr);
  memset(&fmn_genioc,0,sizeof(struct fmn_genioc));
}

/* Signal handler.
 */
 
static void fmn_genioc_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++(fmn_genioc.sigc)>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Performance report and clock.
 */
 
static void fmn_genioc_clock_begin() {
  fmn_genioc.frametime=1.0/FMN_GENIOC_FRAME_RATE;
  fmn_genioc.starttimecpu=fmn_now_cpu_s();
  fmn_genioc.starttime=fmn_now_s();
  fmn_genioc.nexttime=fmn_genioc.starttime;
}

static void fmn_genioc_clock_end() {
  if (fmn_genioc.framec<1) return;
  double endtime=fmn_now_s();
  double endtimecpu=fmn_now_cpu_s();
  double elapsed=endtime-fmn_genioc.starttime;
  double elapsedcpu=endtimecpu-fmn_genioc.starttimecpu;
  fprintf(stderr,
    " %d frames in %.03f s. Average rate %.03f Hz. CPU %.03f. %d clock faults.\n",
    fmn_genioc.framec,elapsed,
    fmn_genioc.framec/elapsed,
    elapsedcpu/elapsed,
    fmn_genioc.clockfaultc
  );
}

/* --help
 */
 
static void fmn_genioc_print_usage() {
  int i;

  fprintf(stderr,
    "\n"
    "Usage: fullmoon [OPTIONS] [CONFIG_FILES]\n"
    "\n"
    "OPTIONS:\n"
    "  --help              Print this message.\n"
    "  --video=NAMES       Video drivers in order of preference.\n"
    "  --audio=NAMES       Audio ''.\n"
    "  --synth=NAMES       Synth ''.\n"
    "  --input=NAMES       Input drivers, all will be initialized.\n"
    "  --audio-rate=HZ     Audio output rate.\n"
    "  --audio-chanc=1|2   Audio channel count.\n"
    "  --audio-device=NAME Audio hardware device, if applicable to driver.\n"
    "  --fullscreen=0|1    Start with fullscreen video.\n"
    "  --width=INT         Window width.\n"
    "  --height=INT        Window height.\n"
    "\n"
  );
  
  #define LIST_DRIVERS(tag) { \
    const struct fmn_hw_##tag##_type *type; \
    for (i=0;type=fmn_hw_##tag##_type_by_index(i);i++) { \
      fprintf(stderr,"  %-15s %s\n",type->name,type->desc); \
    } \
    fprintf(stderr,"\n"); \
  }
  fprintf(stderr,"Video drivers:\n"); LIST_DRIVERS(video)
  fprintf(stderr,"Audio drivers:\n"); LIST_DRIVERS(audio)
  fprintf(stderr,"Synth drivers:\n"); LIST_DRIVERS(synth)
  fprintf(stderr,"Input drivers:\n"); LIST_DRIVERS(input)
  #undef LIST_DRIVERS
}

/* Configuration.
 */
 
static int fmn_genioc_config(struct fmn_hw_mgr *mgr,const char *k,int kc,const char *v,int vc,int vn) {
  if ((kc==4)&&!memcmp(k,"help",4)) { fmn_genioc_print_usage(); fmn_genioc.terminate=1; return 1; }
  return 0;
}

/* Optimization: Grab a few common hw hooks in advance so we can cut out the indirection.
 */
 
static int fmn_genioc_yoink_common_hooks() {

  if (!(fmn_genioc.video=fmn_hw_mgr_get_video(fmn_genioc.mgr))) return -1;
  fmn_genioc.video_begin=fmn_genioc.video->type->begin;
  fmn_genioc.video_end=fmn_genioc.video->type->end;
  
  if (!(fmn_genioc.audio=fmn_hw_mgr_get_audio(fmn_genioc.mgr))) return -1;
  fmn_genioc.audio_lock=fmn_genioc.audio->type->lock;
  fmn_genioc.audio_unlock=fmn_genioc.audio->type->unlock;
  
  return 0;
}

/* Main.
 */
 
int main(int argc,char **argv) {

  signal(SIGINT,fmn_genioc_rcvsig);
  
  struct fmn_inmgr_delegate indelegate={
    .state=fmn_genioc_inmgr_state,
    .action=fmn_genioc_inmgr_action,
  };
  if (!(fmn_genioc.inmgr=fmn_inmgr_new(&indelegate))) return 1;

  struct fmn_hw_delegate delegate={
    .config=fmn_genioc_config,
    .close=fmn_genioc_close,
    .resize=fmn_genioc_resize,
    .focus=fmn_genioc_focus,
    .key=fmn_genioc_key,
    .text=fmn_genioc_text,
    .mmotion=fmn_genioc_mmotion,
    .mbutton=fmn_genioc_mbutton,
    .mwheel=fmn_genioc_mwheel,
    .connect=fmn_genioc_connect,
    .disconnect=fmn_genioc_disconnect,
    .button=fmn_genioc_button,
    .premapped=fmn_genioc_premapped,
  };
  if (!(fmn_genioc.mgr=fmn_hw_mgr_new(&delegate))) return 1;
  if (fmn_hw_mgr_configure_argv(fmn_genioc.mgr,argc,argv)<0) return 1;
  if (fmn_genioc.terminate) return 0;
  if (fmn_hw_mgr_ready(fmn_genioc.mgr)<0) return 1;
  if (fmn_genioc_yoink_common_hooks()<0) return 1;
  
  if (fmn_genioc.video&&fmn_genioc.video->type->provides_system_keyboard) {
    fmn_genioc.devid_keyboard=fmn_hw_devid_next();
    fmn_inmgr_connect(fmn_genioc.inmgr,0,fmn_genioc.devid_keyboard);
  }
  
  setup();
  fmn_genioc_clock_begin();
  
  while (!fmn_genioc.terminate&&!fmn_genioc.sigc) {
    fmn_genioc.framec++;
    
    double now=fmn_now_s();
    if (now>=fmn_genioc.nexttime) {
      fmn_genioc.nexttime+=fmn_genioc.frametime;
      if (fmn_genioc.nexttime<=now) {
        fmn_genioc.clockfaultc++;
        fmn_genioc.nexttime=now+fmn_genioc.frametime;
      }
    } else {
      double sleep_s=fmn_genioc.nexttime-now;
      if (sleep_s>fmn_genioc.frametime) {
        fmn_genioc.clockfaultc++;
        fmn_genioc.nexttime=now+fmn_genioc.frametime;
      } else {
        while (sleep_s>0.0) {
          fmn_sleep_s(sleep_s);
          now=fmn_now_s();
          sleep_s=fmn_genioc.nexttime-now;
        }
        fmn_genioc.nexttime+=fmn_genioc.frametime;
      }
    }
    
    if (fmn_hw_mgr_update(fmn_genioc.mgr)<0) return -1;
    loop();
  }
  
  fmn_genioc_clock_end();
  fmn_genioc_cleanup();
  return 0;
}
