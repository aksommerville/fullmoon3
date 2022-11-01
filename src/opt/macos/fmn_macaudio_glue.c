#include "fmn_macaudio.h"
#include "opt/hw/fmn_hw.h"
#include <string.h>

/* Instance definition and singleton.
 */

struct fmn_hw_audio_macaudio {
  struct fmn_hw_audio hdr;
};

#define AUDIO ((struct fmn_hw_audio_macaudio*)audio)

static struct fmn_hw_audio *fmn_macaudio_global=0;

/* Hooks.
 */

static void _macaudio_cb(int16_t *v,int c) {
  if (fmn_macaudio_global&&fmn_macaudio_global->playing) {
    fmn_macaudio_global->delegate->pcm_out(v,c,fmn_macaudio_global);
  } else {
    memset(v,0,c<<1);
  }
}

static void _macaudio_del(struct fmn_hw_audio *audio) {
  if (audio==fmn_macaudio_global) {
    fmn_macaudio_quit();
    fmn_macaudio_global=0;
  }
}

static int _macaudio_init(struct fmn_hw_audio *audio,const struct fmn_hw_audio_params *params) {
  if (fmn_macaudio_global) return -1;
  if (params) {
    audio->rate=params->rate;
    audio->chanc=params->chanc;
  } else {
    audio->rate=44100;
    audio->chanc=1;
  }
  if (fmn_macaudio_init(audio->rate,audio->chanc,_macaudio_cb)<0) return -1;
  fmn_macaudio_global=audio;
  return 0;
}

static int _macaudio_lock(struct fmn_hw_audio *audio) {
  return fmn_macaudio_lock();
}

static int _macaudio_unlock(struct fmn_hw_audio *audio) {
  return fmn_macaudio_unlock();
}

/* Type definition.
 */

const struct fmn_hw_audio_type fmn_hw_audio_type_macaudio={
  .name="macaudio",
  .desc="AudioUnit for MacOS",
  .objlen=sizeof(struct fmn_hw_audio_macaudio),
  .del=_macaudio_del,
  .init=_macaudio_init,
  .lock=_macaudio_lock,
  .unlock=_macaudio_unlock,
};
