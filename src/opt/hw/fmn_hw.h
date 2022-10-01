/* fmn_hw.h
 * Generic interface to video, audio, input, render, and synth implementations.
 * This interface itself is optional: Specialized systems will want hard-coded implementations for everything, without the generic overhead.
 * Use 'hw' for PCs.
 */
 
#ifndef FMN_HW_H
#define FMN_HW_H

#include <stdint.h>

struct fmn_image;

struct fmn_hw_mgr;
struct fmn_hw_delegate;
struct fmn_hw_video;
struct fmn_hw_audio;
struct fmn_hw_input;
struct fmn_hw_render;
struct fmn_hw_synth;
struct fmn_hw_video_type;
struct fmn_hw_audio_type;
struct fmn_hw_input_type;
struct fmn_hw_render_type;
struct fmn_hw_synth_type;

/* Delegate.
 *******************************************************/
 
struct fmn_hw_delegate {
  void *userdata;
  
  /* Called during configuration for all fields.
   *   >0 to acknowledge, manager will ignore it.
   *    0 if unknown, manager will examine, and fail if still unknown.
   *   -2 for some real error if you fully log it.
   *   <0 for other real errors if you want manager to log it.
   */
  int (*config)(struct fmn_hw_mgr *mgr,const char *k,int kc,const char *v,int vc,int vn);
  
  void (*close)(struct fmn_hw_video *video);
  void (*resize)(struct fmn_hw_video *video,int w,int h);
  void (*focus)(struct fmn_hw_video *video,int focus);
  // Nonzero to ack -- Driver should not attempt to send text for the event then.
  int (*key)(struct fmn_hw_video *video,int keycode,int value);
  void (*text)(struct fmn_hw_video *video,int codepoint);
  void (*mmotion)(struct fmn_hw_video *video,int x,int y);
  void (*mbutton)(struct fmn_hw_video *video,int btnid,int value);
  void (*mwheel)(struct fmn_hw_video *video,int dx,int dy);
  
  // Implement these only if you are not using a 'synth' driver.
  void (*pcm_out)(int16_t *v,int c,struct fmn_hw_audio *audio);
  void (*midi_in)(struct fmn_hw_audio *audio,int devid,const void *v,int c);
  
  void (*connect)(struct fmn_hw_input *input,int devid);
  void (*disconnect)(struct fmn_hw_input *input,int devid);
  void (*button)(struct fmn_hw_input *input,int devid,int btnid,int value);
  // Drivers might have a fixed mapping direct to Full Moon button IDs:
  void (*premapped)(struct fmn_hw_input *input,int devid,uint8_t btnid,int value);
};

/* Manager.
 * Creates and owns all the drivers.
 * You supply one delegate, and we use it with all of the drivers.
 *********************************************************/
 
void fmn_hw_mgr_del(struct fmn_hw_mgr *mgr);

/* 'configure' a manager after creating it, and 'ready' when you're done configuring.
 * At 'ready', we instantiate all the drivers.
 */
struct fmn_hw_mgr *fmn_hw_mgr_new(const struct fmn_hw_delegate *delegate);
int fmn_hw_mgr_configure_argv(struct fmn_hw_mgr *mgr,int argc,char **argv);
int fmn_hw_mgr_configure_file(struct fmn_hw_mgr *mgr,const char *path,int require);
int fmn_hw_mgr_configure_text(struct fmn_hw_mgr *mgr,const char *src,int srcc,const char *refname);
int fmn_hw_mgr_ready(struct fmn_hw_mgr *mgr);

void *fmn_hw_mgr_get_userdata(const struct fmn_hw_mgr *mgr);
struct fmn_hw_video *fmn_hw_mgr_get_video(const struct fmn_hw_mgr *mgr);
struct fmn_hw_audio *fmn_hw_mgr_get_audio(const struct fmn_hw_mgr *mgr);
struct fmn_hw_render *fmn_hw_mgr_get_render(const struct fmn_hw_mgr *mgr);
struct fmn_hw_synth *fmn_hw_mgr_get_synth(const struct fmn_hw_mgr *mgr);
struct fmn_hw_input *fmn_hw_mgr_get_input(const struct fmn_hw_mgr *mgr,int p);

int fmn_hw_mgr_update(struct fmn_hw_mgr *mgr);

/* Video.
 *************************************************************/
 
struct fmn_hw_video_params {
  const char *title;
  const void *iconrgba;
  int iconw,iconh;
  int fullscreen;
  int winw,winh;
};
 
struct fmn_hw_video {
  const struct fmn_hw_video_type *type;
  const struct fmn_hw_delegate *delegate;
  int winw,winh;
  int fullscreen;
};

struct fmn_hw_video_type {
  const char *name;
  const char *desc;
  int by_request_only; // if nonzero, this type will never be chosen by default, only if you ask for it.
  int objlen;
  void (*del)(struct fmn_hw_video *video);
  int (*init)(struct fmn_hw_video *video,const struct fmn_hw_video_params *params);
  int (*update)(struct fmn_hw_video *video);
  
  /* Caller supplies (fb) if software rendering in play.
   * Null if they've written to the global OpenGL context.
   */
  void (*swap)(struct fmn_hw_video *video,struct fmn_image *fb);
  
  void (*set_fullscreen)(struct fmn_hw_video *video,int fullscreen);
  void (*suppress_screensaver)(struct fmn_hw_video *video);
};

void fmn_hw_video_del(struct fmn_hw_video *video);

struct fmn_hw_video *fmn_hw_video_new(
  const struct fmn_hw_video_type *type,
  const struct fmn_hw_delegate *delegate,
  const struct fmn_hw_video_params *params
);

int fmn_hw_video_update(struct fmn_hw_video *video);
void fmn_hw_video_swap(struct fmn_hw_video *video,struct fmn_image *fb);
int fmn_hw_video_set_fullscreen(struct fmn_hw_video *video,int fullscreen);
void fmn_hw_video_suppress_screensaver(struct fmn_hw_video *video);

/* Render.
 ****************************************************************/
 
struct fmn_hw_render_params {
  int fbw,fbh;
  int tilesize;
};
 
struct fmn_hw_render {
  const struct fmn_hw_render_type *type;
  const struct fmn_hw_delegate *delegate;
  int fbw,fbh;
};

struct fmn_hw_render_type {
  const char *name;
  const char *desc;
  int by_request_only;
  int objlen;
  void (*del)(struct fmn_hw_render *render);
  int (*init)(struct fmn_hw_render *render,const struct fmn_hw_render_params *params);
  int (*upload_image)(struct fmn_hw_render *render,uint16_t imageid,struct fmn_image *image);
  int (*begin)(struct fmn_hw_render *render);
  
  /* Return (null) if rendering to OpenGL, otherwise the framebuffer.
   */
  struct fmn_image *(*end)(struct fmn_hw_render *render);
  
  void (*fill_rect)(struct fmn_hw_render *render,int x,int y,int w,int h,uint32_t rgba);
  void (*blit)(
    struct fmn_hw_render *render,int dstx,int dsty,
    uint16_t imageid,int srcx,int srcy,
    int w,int h,
    uint8_t xform
  );
  void (*blit_tile)(struct fmn_hw_render *render,int dstx,int dsty,uint16_t imageid,uint8_t tileid,uint8_t xform);
};

void fmn_hw_render_del(struct fmn_hw_render *render);

struct fmn_hw_render *fmn_hw_render_new(
  const struct fmn_hw_render_type *type,
  const struct fmn_hw_delegate *delegate,
  const struct fmn_hw_render_params *params
);

int fmn_hw_render_upload_image(struct fmn_hw_render *render,uint16_t imageid,struct fmn_image *image);
int fmn_hw_render_begin(struct fmn_hw_render *render);
struct fmn_image *fmn_hw_render_end(struct fmn_hw_render *render);
void fmn_hw_render_fill_rect(struct fmn_hw_render *render,int x,int y,int w,int h,uint32_t rgba);
void fmn_hw_render_blit(
  struct fmn_hw_render *render,int dstx,int dsty,
  uint16_t imageid,int srcx,int srcy,
  int w,int h,
  uint8_t xform
);
void fmn_hw_render_blit_tile(struct fmn_hw_render *render,int dstx,int dsty,uint16_t imageid,uint8_t tileid,uint8_t xform);

/* Audio.
 ********************************************************************/
 
struct fmn_hw_audio_params {
  int rate;
  int chanc;
  char *device;
};
 
struct fmn_hw_audio {
  const struct fmn_hw_audio_type *type;
  const struct fmn_hw_delegate *delegate;
  struct fmn_hw_mgr *mgr; // WEAK
  int rate;
  int chanc;
  int playing;
};

struct fmn_hw_audio_type {
  const char *name;
  const char *desc;
  int by_request_only;
  int objlen;
  void (*del)(struct fmn_hw_audio *audio);
  int (*init)(struct fmn_hw_audio *audio,const struct fmn_hw_audio_params *params);
  int (*play)(struct fmn_hw_audio *audio,int play);
  int (*update)(struct fmn_hw_audio *audio);
  int (*lock)(struct fmn_hw_audio *audio);
  int (*unlock)(struct fmn_hw_audio *audio);
};

void fmn_hw_audio_del(struct fmn_hw_audio *audio);

struct fmn_hw_audio *fmn_hw_audio_new(
  const struct fmn_hw_audio_type *type,
  const struct fmn_hw_delegate *delegate,
  const struct fmn_hw_audio_params *params
);

int fmn_hw_audio_play(struct fmn_hw_audio *audio,int play);
int fmn_hw_audio_update(struct fmn_hw_audio *audio);
int fmn_hw_audio_lock(struct fmn_hw_audio *audio);
int fmn_hw_audio_unlock(struct fmn_hw_audio *audio);

/* Synth.
 *******************************************************************/
 
struct fmn_hw_synth_params {
  int rate;
  int chanc;
};
 
struct fmn_hw_synth {
  const struct fmn_hw_synth_type *type;
  const struct fmn_hw_delegate *delegate;
  int rate;
  int chanc;
};

struct fmn_hw_synth_type {
  const char *name;
  const char *desc;
  int by_request_only;
  int objlen;
  void (*del)(struct fmn_hw_synth *synth);
  int (*init)(struct fmn_hw_synth *synth,const struct fmn_hw_synth_params *params);
  void (*update)(int16_t *v,int c,struct fmn_hw_synth *synth);
  int (*configure)(struct fmn_hw_synth *synth,const void *v,int c);
  int (*serial_midi)(struct fmn_hw_synth *synth,const void *v,int c);
  int (*play_song)(struct fmn_hw_synth *synth,const void *v,int c);
  void (*note)(struct fmn_hw_synth *synth,uint8_t programid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms);
  void (*silence)(struct fmn_hw_synth *synth);
  void (*release_all)(struct fmn_hw_synth *synth);
};

void fmn_hw_synth_del(struct fmn_hw_synth *synth);

struct fmn_hw_synth *fmn_hw_synth_new(
  const struct fmn_hw_synth_type *type,
  const struct fmn_hw_delegate *delegate,
  const struct fmn_hw_synth_params *params
);

void fmn_hw_synth_update(int16_t *v,int c,struct fmn_hw_synth *synth);

int fmn_hw_synth_configure(struct fmn_hw_synth *synth,const void *v,int c);

// Process MIDI events direct from the bus.
int fmn_hw_synth_serial_midi(struct fmn_hw_synth *synth,const void *v,int c);

/* Caller must ensure that (v) remain valid and constant as long as it's playing.
 */
int fmn_hw_synth_play_song(struct fmn_hw_synth *synth,const void *v,int c);

void fmn_hw_synth_note(struct fmn_hw_synth *synth,uint8_t programid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms);
void fmn_hw_synth_silence(struct fmn_hw_synth *synth);
void fmn_hw_synth_release_all(struct fmn_hw_synth *synth);

/* Input.
 ****************************************************************/
 
struct fmn_hw_input {
  const struct fmn_hw_input_type *type;
  const struct fmn_hw_delegate *delegate;
};

struct fmn_hw_input_type {
  const char *name;
  const char *desc;
  int by_request_only;
  int objlen;
  void (*del)(struct fmn_hw_input *input);
  int (*init)(struct fmn_hw_input *input);
  int (*update)(struct fmn_hw_input *input);
  const char *(*get_ids)(int *vid,int *pid,struct fmn_hw_input *input,int devid);
  int (*enumerate)(
    struct fmn_hw_input *input,
    int devid,
    int (*cb)(struct fmn_hw_input *input,int devid,int btnid,int usage,int lo,int hi,int value,void *userdata),
    void *userdata
  );
};

void fmn_hw_input_del(struct fmn_hw_input *input);

struct fmn_hw_input *fmn_hw_input_new(
  const struct fmn_hw_input_type *type,
  const struct fmn_hw_delegate *delegate
);

int fmn_hw_input_update(struct fmn_hw_input *input);

const char *fmn_hw_input_get_ids(int *vid,int *pid,struct fmn_hw_input *input,int devid);

int fmn_hw_input_enumerate(
  struct fmn_hw_input *input,
  int devid,
  int (*cb)(struct fmn_hw_input *input,int devid,int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
);

/* Registries.
 ******************************************************************/
 
const struct fmn_hw_video_type *fmn_hw_video_type_by_name(const char *name,int namec);
const struct fmn_hw_video_type *fmn_hw_video_type_by_index(int p);
const struct fmn_hw_audio_type *fmn_hw_audio_type_by_name(const char *name,int namec);
const struct fmn_hw_audio_type *fmn_hw_audio_type_by_index(int p);
const struct fmn_hw_input_type *fmn_hw_input_type_by_name(const char *name,int namec);
const struct fmn_hw_input_type *fmn_hw_input_type_by_index(int p);
const struct fmn_hw_render_type *fmn_hw_render_type_by_name(const char *name,int namec);
const struct fmn_hw_render_type *fmn_hw_render_type_by_index(int p);
const struct fmn_hw_synth_type *fmn_hw_synth_type_by_name(const char *name,int namec);
const struct fmn_hw_synth_type *fmn_hw_synth_type_by_index(int p);

int fmn_hw_devid_next();

#endif
