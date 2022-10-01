#include "fmn_alsa_internal.h"

/* I/O thread.
 */
 
static void *fmn_alsa_iothd(void *arg) {
  struct fmn_hw_audio *audio=arg;
  while (1) {
    pthread_testcancel();
    
    if (pthread_mutex_lock(&AUDIO->iomtx)) {
      usleep(1000);
      continue;
    }
    if (audio->playing&&audio->delegate->pcm_out) {
      audio->delegate->pcm_out(AUDIO->buf,AUDIO->bufa,audio);
    } else {
      memset(AUDIO->buf,0,AUDIO->bufa<<1);
    }
    pthread_mutex_unlock(&AUDIO->iomtx);
    
    int framec=AUDIO->bufa_frames;
    int framep=0;
    while (framep<framec) {
      pthread_testcancel();
      int pvcancel;
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pvcancel);
      int err=snd_pcm_writei(AUDIO->alsa,AUDIO->buf+framep*audio->chanc,framec-framep);
      pthread_setcancelstate(pvcancel,0);
      if (err<=0) {
        if (snd_pcm_recover(AUDIO->alsa,err,0)<0) return 0;
        break;
      }
      framep+=err;
    }
  }
}

/* Cleanup.
 */
 
static void _alsa_del(struct fmn_hw_audio *audio) {
  if (AUDIO->iothd) {
    pthread_cancel(AUDIO->iothd);
    pthread_join(AUDIO->iothd,0);
  }
  if (AUDIO->alsa) snd_pcm_close(AUDIO->alsa);
  if (AUDIO->buf) free(AUDIO->buf);
}

/* Init.
 */
 
static int _alsa_init(struct fmn_hw_audio *audio,const struct fmn_hw_audio_params *params) {
  
  const char *device=0;
  if (params) {
    audio->rate=params->rate;
    audio->chanc=params->chanc;
    device=params->device;
  }
       if (audio->rate<FMN_ALSA_RATE_MIN) audio->rate=FMN_ALSA_RATE_MIN;
  else if (audio->rate>FMN_ALSA_RATE_MAX) audio->rate=FMN_ALSA_RATE_MAX;
       if (audio->chanc<FMN_ALSA_CHANC_MIN) audio->chanc=FMN_ALSA_CHANC_MIN;
  else if (audio->chanc>FMN_ALSA_CHANC_MAX) audio->chanc=FMN_ALSA_CHANC_MAX;
  if (!device) device=FMN_ALSA_DEFAULT_DEVICE;

  AUDIO->bufa_frames=audio->rate/30;

  snd_pcm_hw_params_t *hwparams=0;
  if (
    (snd_pcm_open(&AUDIO->alsa,device,SND_PCM_STREAM_PLAYBACK,0)<0)||
    (snd_pcm_hw_params_malloc(&hwparams)<0)||
    (snd_pcm_hw_params_any(AUDIO->alsa,hwparams)<0)||
    (snd_pcm_hw_params_set_access(AUDIO->alsa,hwparams,SND_PCM_ACCESS_RW_INTERLEAVED)<0)||
    (snd_pcm_hw_params_set_format(AUDIO->alsa,hwparams,SND_PCM_FORMAT_S16)<0)||
    (snd_pcm_hw_params_set_rate_near(AUDIO->alsa,hwparams,&audio->rate,0)<0)||
    (snd_pcm_hw_params_set_channels_near(AUDIO->alsa,hwparams,&audio->chanc)<0)||
    (snd_pcm_hw_params_set_buffer_size_near(AUDIO->alsa,hwparams,&AUDIO->bufa_frames)<0)||
    (snd_pcm_hw_params(AUDIO->alsa,hwparams)<0)
  ) {
    snd_pcm_hw_params_free(hwparams);
    return -1;
  }
  
  snd_pcm_hw_params_free(hwparams);
  
  if (snd_pcm_nonblock(AUDIO->alsa,0)<0) return -1;
  if (snd_pcm_prepare(AUDIO->alsa)<0) return -1;

  AUDIO->bufa=AUDIO->bufa_frames*audio->chanc;
  if (!(AUDIO->buf=malloc(AUDIO->bufa*2))) return -1;

  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_settype(&mattr,PTHREAD_MUTEX_RECURSIVE);
  if (pthread_mutex_init(&AUDIO->iomtx,&mattr)) return -1;
  pthread_mutexattr_destroy(&mattr);
  if (pthread_create(&AUDIO->iothd,0,fmn_alsa_iothd,audio)) return -1;
  
  return 0;
}

/* Lock.
 */
 
static int _alsa_lock(struct fmn_hw_audio *audio) {
  if (pthread_mutex_lock(&AUDIO->iomtx)) return -1;
  return 0;
}

static int _alsa_unlock(struct fmn_hw_audio *audio) {
  if (pthread_mutex_unlock(&AUDIO->iomtx)) return -1;
  return 0;
}

/* Type definition.
 */
 
const struct fmn_hw_audio_type fmn_hw_audio_type_alsa={
  .name="alsa",
  .desc="Audio for Linux via ALSA, preferred for non-GUI systems.",
  .objlen=sizeof(struct fmn_hw_audio_alsa),
  .del=_alsa_del,
  .init=_alsa_init,
  .lock=_alsa_lock,
  .unlock=_alsa_unlock,
};
