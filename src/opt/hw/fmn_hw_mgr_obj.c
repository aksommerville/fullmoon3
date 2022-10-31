#include "fmn_hw_internal.h"

/* Delete.
 */
 
void fmn_hw_mgr_del(struct fmn_hw_mgr *mgr) {
  if (!mgr) return;
  
  if (mgr->audio) {
    fmn_hw_audio_play(mgr->audio,0);
    fmn_hw_audio_del(mgr->audio);
  }
  fmn_hw_video_del(mgr->video);
  fmn_hw_synth_del(mgr->synth);
  if (mgr->inputv) {
    while (mgr->inputc-->0) fmn_hw_input_del(mgr->inputv[mgr->inputc]);
    free(mgr->inputv);
  }
  
  if (mgr->audio_params.device) free(mgr->audio_params.device);
  if (mgr->video_names) free(mgr->video_names);
  if (mgr->audio_names) free(mgr->audio_names);
  if (mgr->input_names) free(mgr->input_names);
  if (mgr->synth_names) free(mgr->synth_names);
  
  free(mgr);
}

/* New.
 */

struct fmn_hw_mgr *fmn_hw_mgr_new(const struct fmn_hw_delegate *delegate) {
  struct fmn_hw_mgr *mgr=calloc(1,sizeof(struct fmn_hw_mgr));
  if (!mgr) return 0;
  if (delegate) mgr->delegate=*delegate;
  return mgr;
}

/* Trivial accessors.
 */

void *fmn_hw_mgr_get_userdata(const struct fmn_hw_mgr *mgr) {
  return mgr->delegate.userdata;
}

struct fmn_hw_video *fmn_hw_mgr_get_video(const struct fmn_hw_mgr *mgr) {
  return mgr->video;
}

struct fmn_hw_audio *fmn_hw_mgr_get_audio(const struct fmn_hw_mgr *mgr) {
  return mgr->audio;
}

struct fmn_hw_synth *fmn_hw_mgr_get_synth(const struct fmn_hw_mgr *mgr) {
  return mgr->synth;
}

struct fmn_hw_input *fmn_hw_mgr_get_input(const struct fmn_hw_mgr *mgr,int p) {
  if (p<0) return 0;
  if (p>=mgr->inputc) return 0;
  return mgr->inputv[p];
}

/* Update.
 */

int fmn_hw_mgr_update(struct fmn_hw_mgr *mgr) {
  if (mgr->video) {
    if (fmn_hw_video_update(mgr->video)<0) return -1;
  }
  if (mgr->audio) {
    if (fmn_hw_audio_update(mgr->audio)<0) return -1;
  }
  int i=mgr->inputc;
  struct fmn_hw_input **input=mgr->inputv;
  for (;i-->0;input++) {
    if (fmn_hw_input_update(*input)<0) {
      //fprintf(stderr,"fmn_hw_input_update failed\n");
      return -1;
    }
  }
  return 0;
}

/* Audio callbacks.
 */
 
void fmn_hw_mgr_pcm_out(int16_t *v,int c,struct fmn_hw_audio *audio) {
  struct fmn_hw_mgr *mgr=audio->mgr;
  if (mgr->synth) {
    fmn_hw_synth_update(v,c,mgr->synth);
  } else {
    memset(v,0,c<<1);
  }
}

void fmn_hw_mgr_midi_in(struct fmn_hw_audio *audio,int devid,const void *v,int c) {
  struct fmn_hw_mgr *mgr=audio->mgr;
  if (mgr->synth) {
    fmn_hw_audio_lock(audio);
    fmn_hw_synth_serial_midi(mgr->synth,v,c);
    fmn_hw_audio_unlock(audio);
  }
}
