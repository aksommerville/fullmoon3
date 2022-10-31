#include "fmn_macioc_internal.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

struct fmn_macioc fmn_macioc={0};

/* Log to a text file. Will work even if the TTY is unset.
 */
#if 0 // I'll just leave this here in case we need it again...
static void fmn_macioc_surelog(const char *fmt,...) {
  va_list vargs;
  va_start(vargs,fmt);
  char message[256];
  int messagec=vsnprintf(message,sizeof(message),fmt,vargs);
  if ((messagec<0)||(messagec>=sizeof(message))) {
    messagec=snprintf(message,sizeof(message),"(unable to generate message)");
  }
  int f=open("/Users/andy/proj/fullmoon/surelog",O_WRONLY|O_APPEND|O_CREAT,0666);
  if (f<0) return;
  int err=write(f,message,messagec);
  close(f);
}

#define SURELOG(fmt,...) fmn_macioc_surelog("%d:%s:%d:%s: "fmt"\n",(int)time(0),__FILE__,__LINE__,__func__,##__VA_ARGS__);
#endif

/* Reopen TTY.
 */
 
static int fmn_macioc_reopen_tty(const char *path) {
  int fd=open(path,O_RDWR);
  if (fd<0) return -1;
  dup2(fd,STDIN_FILENO);
  dup2(fd,STDOUT_FILENO);
  dup2(fd,STDERR_FILENO);
  close(fd);
  return 0;
}

/* First pass through argv.
 */

static int fmn_macioc_argv_prerun(int argc,char **argv) {

  // argv[0] will have the full path to the executable. A bit excessive.
  if ((argc>=1)&&argv[0]) {
    char *src=argv[0];
    char *base=src;
    for (;*src;src++) if (*src=='/') base=src+1;
    argv[0]=base;
  }

  int argp;
  for (argp=1;argp<argc;argp++) {
    const char *k=argv[argp];
    if (!k) continue;
    if ((k[0]!='-')||(k[1]!='-')||!k[2]) continue;
    k+=2;
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    const char *v=k+kc;
    int vc=0;
    if (v[0]=='=') {
      v++;
      while (v[vc]) vc++;
    }

    if ((kc==10)&&!memcmp(k,"reopen-tty",10)) {
      if (fmn_macioc_reopen_tty(v)<0) return -1;
      argv[argp]="";

    } else if ((kc==5)&&!memcmp(k,"chdir",5)) {
      if (chdir(v)<0) return -1;
      argv[argp]="";

    }
  }
  return 0;
}

/* Configure.
 */

static int fmn_macioc_configure(int argc,char **argv) {
  int argp;

  if (fmn_macioc_argv_prerun(argc,argv)<0) return -1;
  fmn_macioc.argc=argc;
  fmn_macioc.argv=argv;

  #if FMN_USE_inmap
    fmn_macioc.inmap=fmn_inmap_new();
    //TODO config file
  #endif

  return 0;
}

/* Main.
 */

int main(int argc,char **argv) {

  if (fmn_macioc.init) return 1;
  memset(&fmn_macioc,0,sizeof(struct fmn_macioc));
  fmn_macioc.init=1;

  if (fmn_macioc_configure(argc,argv)<0) return 1;

  return NSApplicationMain(argc,(const char**)argv);
}

/* Abort.
 */
 
void fmn_macioc_abort(const char *fmt,...) {
  if (fmt&&fmt[0]) {
    va_list vargs;
    va_start(vargs,fmt);
    char msg[256];
    int msgc=vsnprintf(msg,sizeof(msg),fmt,vargs);
    if ((msgc<0)||(msgc>=sizeof(msg))) msgc=0;
    fprintf(stderr,"%.*s\n",msgc,msg);
  }
  [NSApplication.sharedApplication terminate:nil];
  fprintf(stderr,"!!! [NSApplication.sharedApplication terminate:nil] did not terminate execution. Using exit() instead !!!\n");
  exit(1);
}

/* --help
 */
 
static void fmn_macioc_print_usage() {
  int i;

  fprintf(stderr,
    "\n"
    "Usage: fullmoon [OPTIONS] [CONFIG_FILES]\n"
    "Or more likely: open -W FullMoon.app --args --reopen-tty=$(tty) --chdir=$(pwd) [OPTIONS] [CONFIG_FILES]\n"
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
    "  --tilesize=INT      Tile size: 5 8 16 32\n"
    "  --fbfmt=FORMAT      Framebuffer format (see below)\n"
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
  
  fprintf(stderr,"Framebuffer formats:\n");
  #define _(tag) if (FMN_IMAGE_FMT_##tag!=FMN_IMAGE_FMT_ENCODED) fprintf(stderr,"  %s\n",#tag);
  FMN_FOR_EACH_IMAGE_FMT
  #undef _
  fprintf(stderr,"\n");
}

/* Configuration.
 */
 
static int fmn_macioc_config(struct fmn_hw_mgr *mgr,const char *k,int kc,const char *v,int vc,int vn) {
  if ((kc==4)&&!memcmp(k,"help",4)) { fmn_macioc_print_usage(); fmn_macioc.terminate=1; return 1; }
  return 0;
}

/* Start up, after NSApplication does its thing.
 */
 
static int fmn_macioc_init() {

  struct fmn_hw_delegate delegate={
    .userdata=0,
    .config=fmn_macioc_config,
    .close=fmn_macioc_cb_close,
    .focus=fmn_macioc_cb_focus,
    .resize=fmn_macioc_cb_resize,
    .key=fmn_macioc_cb_key,
    .text=fmn_macioc_cb_text,
    .mmotion=fmn_macioc_cb_mmotion,
    .mbutton=fmn_macioc_cb_mbutton,
    .mwheel=fmn_macioc_cb_mwheel,
    .connect=fmn_macioc_cb_connect,
    .disconnect=fmn_macioc_cb_disconnect,
    .button=fmn_macioc_cb_event,
    .premapped=fmn_macioc_cb_premapped_event,
  };

  if (!(fmn_macioc.hw=fmn_hw_mgr_new(&delegate))) return -1;
  if (fmn_hw_mgr_configure_argv(fmn_macioc.hw,fmn_macioc.argc,fmn_macioc.argv)<0) return -1;
  if (fmn_macioc.terminate) {
    [NSApplication.sharedApplication terminate:0];
    return 0;
  }
  if (fmn_hw_mgr_ready(fmn_macioc.hw)<0) {
    fprintf(stderr,"Error initializing drivers.\n");
    return -1;
  }
  
  setup();

  return 0;
}

/* Entry points from game.
 */

// Not necessary; we do the initting and updating out-of-band.
int8_t fmn_platform_init() { return 0; }
uint8_t fmn_platform_update() { return fmn_macioc.input; }

uint8_t fmn_platform_video_get_format() {
  struct fmn_hw_video *video=fmn_hw_mgr_get_video(fmn_macioc.hw);
  if (video) return video->fbfmt;
  return FMN_IMAGE_FMT_Y2;
}
 
struct fmn_image *fmn_platform_video_begin() {
  struct fmn_hw_video *video=fmn_hw_mgr_get_video(fmn_macioc.hw);
  if (video) return fmn_hw_video_begin(video);
  return 0;
}

void fmn_platform_video_end(struct fmn_image *fb) {
  struct fmn_hw_video *video=fmn_hw_mgr_get_video(fmn_macioc.hw);
  if (video) fmn_hw_video_end(video,fb);
}

void fmn_platform_audio_configure(const void *v,uint16_t c) {
  struct fmn_hw_audio *audio=fmn_hw_mgr_get_audio(fmn_macioc.hw);
  if (!audio) return;
  if (fmn_hw_audio_lock(audio)<0) return;
  fmn_hw_synth_configure(fmn_hw_mgr_get_synth(fmn_macioc.hw),v,c);
  fmn_hw_audio_unlock(audio);
}

void fmn_platform_audio_play_song(const void *v,uint16_t c) {
  struct fmn_hw_audio *audio=fmn_hw_mgr_get_audio(fmn_macioc.hw);
  if (!audio) return;
  if (fmn_hw_audio_lock(audio)<0) return;
  fmn_hw_synth_play_song(fmn_hw_mgr_get_synth(fmn_macioc.hw),v,c);
  fmn_hw_audio_unlock(audio);
}

void fmn_platform_audio_pause_song(uint8_t pause) {
  struct fmn_hw_audio *audio=fmn_hw_mgr_get_audio(fmn_macioc.hw);
  if (!audio) return;
  if (fmn_hw_audio_lock(audio)<0) return;
  fmn_hw_synth_pause_song(fmn_hw_mgr_get_synth(fmn_macioc.hw),pause);
  fmn_hw_audio_unlock(audio);
}

void fmn_platform_audio_note(uint8_t programid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms) {
  struct fmn_hw_audio *audio=fmn_hw_mgr_get_audio(fmn_macioc.hw);
  if (!audio) return;
  if (fmn_hw_audio_lock(audio)<0) return;
  fmn_hw_synth_note(fmn_hw_mgr_get_synth(fmn_macioc.hw),programid,noteid,velocity,duration_ms);
  fmn_hw_audio_unlock(audio);
}

void fmn_platform_audio_silence() {
  struct fmn_hw_audio *audio=fmn_hw_mgr_get_audio(fmn_macioc.hw);
  if (!audio) return;
  if (fmn_hw_audio_lock(audio)<0) return;
  fmn_hw_synth_silence(fmn_hw_mgr_get_synth(fmn_macioc.hw));
  fmn_hw_audio_unlock(audio);
}

void fmn_platform_audio_release_all() {
  struct fmn_hw_audio *audio=fmn_hw_mgr_get_audio(fmn_macioc.hw);
  if (!audio) return;
  if (fmn_hw_audio_lock(audio)<0) return;
  fmn_hw_synth_release_all(fmn_hw_mgr_get_synth(fmn_macioc.hw));
  fmn_hw_audio_unlock(audio);
}

/* Final performance report.
 */
 
static void fmn_macioc_report_performance() {
  if (fmn_macioc.framec<1) return;
  double elapsed=fmn_macioc_now_s()-fmn_macioc.starttime;
  double elapsedcpu=fmn_macioc_now_cpu_s()-fmn_macioc.starttimecpu;
  fprintf(stderr,
    "%d frames in %.03f s, average %.03f Hz. CPU=%.06f\n",
    fmn_macioc.framec,elapsed,fmn_macioc.framec/elapsed,elapsedcpu/elapsed
  );
}

@implementation AKAppDelegate

/* Main loop.
 * This runs on a separate thread.
 */

-(void)mainLoop:(id)ignore {

  fmn_macioc.frametime=1000000/FMN_MACIOC_FRAME_RATE;
  fmn_macioc.starttime=fmn_macioc_now_s();
  fmn_macioc.starttimecpu=fmn_macioc_now_cpu_s();
  fmn_macioc.nexttime=fmn_macioc_now_us();
  
  while (1) {
    //fprintf(stderr,"%s:%d\n",__FILE__,__LINE__);

    if (fmn_macioc.terminate) break;

    int64_t now=fmn_macioc_now_us();
    while (1) {
      int64_t sleeptime=fmn_macioc.nexttime-now;
      if (sleeptime<=0) {
        fmn_macioc.nexttime+=fmn_macioc.frametime;
        if (fmn_macioc.nexttime<=now) { // panic
          fmn_macioc.nexttime=now+fmn_macioc.frametime;
        }
        break;
      }
      if (sleeptime>FMN_MACIOC_SLEEP_LIMIT) { // panic
        fmn_macioc.nexttime=now+fmn_macioc.frametime;
        break;
      }
      fmn_macioc_sleep_us(sleeptime);
      now=fmn_macioc_now_us();
    }

    if (fmn_macioc.terminate) break;

    if (fmn_macioc.update_in_progress) {
      //fmn_log(MACIOC,TRACE,"Dropping frame due to update still running.");
      continue;
    }

    /* With 'waitUntilDone:0', we will always be on manual timing.
     * I think that's OK. And window resizing is much more responsive this way.
     * Update:
     *   !!! After upgrading from 10.11 to 10.13, all the timing got fucked.
     *   Switching to 'waitUntilDone:1' seems to fix it.
     *   If the only problem that way in 10.11 was choppy window resizing, so be it.
     *   Resize seems OK with '1' and OS 10.13.
     */
    [self performSelectorOnMainThread:@selector(updateMain:) withObject:nil waitUntilDone:1];
  
  }
}

/* Route call from main loop.
 * This runs on the main thread.
 */

-(void)updateMain:(id)ignore {
  fmn_macioc.update_in_progress=1;
  fmn_macioc.framec++;
  if (fmn_hw_mgr_update(fmn_macioc.hw)<0) {
    fmn_macioc_abort("fmn_hw_mgr_update failed");
    return;
  }
  loop();
  fmn_macioc.update_in_progress=0;
}

/* Finish launching.
 * We fire the 'init' callback and launch an updater thread.
 */

-(void)applicationDidFinishLaunching:(NSNotification*)notification {
  int err=fmn_macioc_init();
  if (err<0) {
    fmn_macioc_abort("Initialization failed (%d). Aborting.",err);
  }
  [NSThread detachNewThreadSelector:@selector(mainLoop:) toTarget:self withObject:nil];
}

/* Termination.
 */

-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
  return NSTerminateNow;
}

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  return 1;
}

-(void)applicationWillTerminate:(NSNotification*)notification {
  fmn_macioc.terminate=1;
  fmn_macioc_report_performance();
  fmn_hw_mgr_del(fmn_macioc.hw);
  fmn_macioc.hw=0;
}

/* Receive system error.
 */

-(NSError*)application:(NSApplication*)application willPresentError:(NSError*)error {
  const char *message=error.localizedDescription.UTF8String;
  fprintf(stderr,"%s\n",message);
  return error;
}

/* Change input focus.
 * intf expects this to be a video concern, but on the Mac, it's an Application thing. Defer.
 */

-(void)applicationDidBecomeActive:(NSNotification*)notification {
  if (fmn_macioc.hw) fmn_macioc_cb_focus(fmn_hw_mgr_get_video(fmn_macioc.hw),1);
}

-(void)applicationDidResignActive:(NSNotification*)notification {
  if (fmn_macioc.hw) fmn_macioc_cb_focus(fmn_hw_mgr_get_video(fmn_macioc.hw),0);
}

@end
