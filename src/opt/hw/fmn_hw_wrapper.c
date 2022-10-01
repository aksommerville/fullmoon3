#include "fmn_hw.h"
#include <stdlib.h>

void fmn_hw_video_del(struct fmn_hw_video *video) {
  if (!video) return;
  if (video->type->del) video->type->del(video);
  free(video);
}

struct fmn_hw_video *fmn_hw_video_new(
  const struct fmn_hw_video_type *type,
  const struct fmn_hw_delegate *delegate,
  const struct fmn_hw_video_params *params
) {
  if (!type||!delegate) return 0;
  struct fmn_hw_video *video=calloc(1,type->objlen);
  if (!video) return 0;
  video->type=type;
  video->delegate=delegate;
  if (type->init&&(type->init(video,params)<0)) {
    fmn_hw_video_del(video);
    return 0;
  }
  return video;
}

int fmn_hw_video_update(struct fmn_hw_video *video) {
  if (!video->type->update) return 0;
  return video->type->update(video);
}

void fmn_hw_video_swap(struct fmn_hw_video *video,struct fmn_image *fb) {
  if (!video->type->swap) return;
  video->type->swap(video,fb);
}

int fmn_hw_video_set_fullscreen(struct fmn_hw_video *video,int fullscreen) {
  if (fullscreen) {
    if (video->fullscreen) return 1;
    if (video->type->set_fullscreen) video->type->set_fullscreen(video,1);
  } else {
    if (!video->fullscreen) return 0;
    if (video->type->set_fullscreen) video->type->set_fullscreen(video,0);
  }
  return video->fullscreen;
}

void fmn_hw_video_suppress_screensaver(struct fmn_hw_video *video) {
  if (video->type->suppress_screensaver) video->type->suppress_screensaver(video);
}

void fmn_hw_render_del(struct fmn_hw_render *render) {
  if (!render) return;
  if (render->type->del) render->type->del(render);
  free(render);
}

struct fmn_hw_render *fmn_hw_render_new(
  const struct fmn_hw_render_type *type,
  const struct fmn_hw_delegate *delegate,
  const struct fmn_hw_render_params *params
) {
  if (!type||!delegate) return 0;
  struct fmn_hw_render *render=calloc(1,type->objlen);
  if (!render) return 0;
  render->type=type;
  render->delegate=delegate;
  if (type->init&&(type->init(render,params)<0)) {
    fmn_hw_render_del(render);
    return 0;
  }
  return render;
}

int fmn_hw_render_upload_image(struct fmn_hw_render *render,uint16_t imageid,struct fmn_image *image) {
  if (!render->type->upload_image) return -1;
  return render->type->upload_image(render,imageid,image);
}

int fmn_hw_render_begin(struct fmn_hw_render *render) {
  if (!render->type->begin) return 0;
  return render->type->begin(render);
}

struct fmn_image *fmn_hw_render_end(struct fmn_hw_render *render) {
  if (!render->type->end) return 0;
  return render->type->end(render);
}

void fmn_hw_render_fill_rect(struct fmn_hw_render *render,int x,int y,int w,int h,uint32_t rgba) {
  if (!render->type->fill_rect) return;
  render->type->fill_rect(render,x,y,w,h,rgba);
}

void fmn_hw_render_blit(
  struct fmn_hw_render *render,int dstx,int dsty,
  uint16_t imageid,int srcx,int srcy,
  int w,int h,
  uint8_t xform
) {
  if (!render->type->blit) return;
  render->type->blit(render,dstx,dsty,imageid,srcx,srcy,w,h,xform);
}

void fmn_hw_render_blit_tile(struct fmn_hw_render *render,int dstx,int dsty,uint16_t imageid,uint8_t tileid,uint8_t xform) {
  if (!render->type->blit_tile) return;
  render->type->blit_tile(render,dstx,dsty,imageid,tileid,xform);
}

void fmn_hw_audio_del(struct fmn_hw_audio *audio) {
  if (!audio) return;
  if (audio->type->del) audio->type->del(audio);
  free(audio);
}

struct fmn_hw_audio *fmn_hw_audio_new(
  const struct fmn_hw_audio_type *type,
  const struct fmn_hw_delegate *delegate,
  const struct fmn_hw_audio_params *params
) {
  if (!type||!delegate) return 0;
  struct fmn_hw_audio *audio=calloc(1,type->objlen);
  if (!audio) return 0;
  audio->type=type;
  audio->delegate=delegate;
  if (type->init&&(type->init(audio,params)<0)) {
    fmn_hw_audio_del(audio);
    return 0;
  }
  return audio;
}

int fmn_hw_audio_play(struct fmn_hw_audio *audio,int play) {
  if (!audio->type->play) {
    audio->playing=play?1:0;
    return 0;
  }
  return audio->type->play(audio,play);
}

int fmn_hw_audio_update(struct fmn_hw_audio *audio) {
  if (!audio->type->update) return 0;
  return audio->type->update(audio);
}

int fmn_hw_audio_lock(struct fmn_hw_audio *audio) {
  if (!audio->type->lock) return 0;
  return audio->type->lock(audio);
}

int fmn_hw_audio_unlock(struct fmn_hw_audio *audio) {
  if (!audio->type->unlock) return 0;
  return audio->type->unlock(audio);
}

void fmn_hw_synth_del(struct fmn_hw_synth *synth) {
  if (!synth) return;
  if (synth->type->del) synth->type->del(synth);
  free(synth);
}

struct fmn_hw_synth *fmn_hw_synth_new(
  const struct fmn_hw_synth_type *type,
  const struct fmn_hw_delegate *delegate,
  const struct fmn_hw_synth_params *params
) {
  if (!type||!delegate) return 0;
  struct fmn_hw_synth *synth=calloc(1,type->objlen);
  if (!synth) return 0;
  synth->type=type;
  synth->delegate=delegate;
  if (type->init&&(type->init(synth,params)<0)) {
    fmn_hw_synth_del(synth);
    return 0;
  }
  return synth;
}

void fmn_hw_synth_update(int16_t *v,int c,struct fmn_hw_synth *synth) {
  if (!synth->type->update) return;
  synth->type->update(v,c,synth);
}

int fmn_hw_synth_configure(struct fmn_hw_synth *synth,const void *v,int c) {
  if (!synth->type->configure) return 0;
  return synth->type->configure(synth,v,c);
}

int fmn_hw_synth_serial_midi(struct fmn_hw_synth *synth,const void *v,int c) {
  if (!synth->type->serial_midi) return -1;
  return synth->type->serial_midi(synth,v,c);
}

int fmn_hw_synth_play_song(struct fmn_hw_synth *synth,const void *v,int c) {
  if (!synth->type->play_song) return -1;
  return synth->type->play_song(synth,v,c);
}

void fmn_hw_synth_note(struct fmn_hw_synth *synth,uint8_t programid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms) {
  if (!synth->type->note) return;
  return synth->type->note(synth,programid,noteid,velocity,duration_ms);
}

void fmn_hw_synth_silence(struct fmn_hw_synth *synth) {
  if (!synth->type->silence) return;
  synth->type->silence(synth);
}

void fmn_hw_synth_release_all(struct fmn_hw_synth *synth) {
  if (!synth->type->release_all) return;
  synth->type->release_all(synth);
}

void fmn_hw_input_del(struct fmn_hw_input *input) {
  if (!input) return;
  if (input->type->del) input->type->del(input);
  free(input);
}

struct fmn_hw_input *fmn_hw_input_new(
  const struct fmn_hw_input_type *type,
  const struct fmn_hw_delegate *delegate
) {
  if (!type||!delegate) return 0;
  struct fmn_hw_input *input=calloc(1,type->objlen);
  if (!input) return 0;
  input->type=type;
  input->delegate=delegate;
  if (type->init&&(type->init(input)<0)) {
    fmn_hw_input_del(input);
    return 0;
  }
  return input;
}

int fmn_hw_input_update(struct fmn_hw_input *input) {
  if (!input->type->update) return 0;
  return input->type->update(input);
}

const char *fmn_hw_input_get_ids(int *vid,int *pid,struct fmn_hw_input *input,int devid) {
  if (!input->type->get_ids) return 0;
  return input->type->get_ids(vid,pid,input,devid);
}

int fmn_hw_input_enumerate(
  struct fmn_hw_input *input,
  int devid,
  int (*cb)(struct fmn_hw_input *input,int devid,int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  if (!input->type->enumerate) return 0;
  return input->type->enumerate(input,devid,cb,userdata);
}
