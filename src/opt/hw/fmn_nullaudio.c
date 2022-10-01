#include "fmn_hw.h"
#include "opt/time/fmn_time.h"

// Never generate more than this in one update; drop any excess.
#define FMN_NULLAUDIO_SANITY_LIMIT_FRAMES 10000

// Must be a multiple of all legal channel counts.
// (I'm planning to keep those at 1 and 2, so hey easy).
#define FMN_NULLAUDIO_BUFFER_SIZE_SAMPLES 1024

#define FMN_NULLAUDIO_RATE_MIN 200
#define FMN_NULLAUDIO_RATE_MAX 200000
#define FMN_NULLAUDIO_CHANC_MIN 1
#define FMN_NULLAUDIO_CHANC_MAX 2

/* Object definition.
 */
 
struct fmn_hw_audio_nullaudio {
  struct fmn_hw_audio hdr;
  double lasttime;
  int16_t buf[FMN_NULLAUDIO_BUFFER_SIZE_SAMPLES];
};

#define AUDIO ((struct fmn_hw_audio_nullaudio*)audio)

/* Delete.
 */
 
static void _nullaudio_del(struct fmn_hw_audio *audio) {
}

/* Init.
 */
 
static int _nullaudio_init(struct fmn_hw_audio *audio,const struct fmn_hw_audio_params *params) {
  // Don't initialize the clock here; leave it for the first update.
  // (there might be lots of heavy loading going on just now)
  
  if (params) {
    audio->rate=params->rate;
    audio->chanc=params->chanc;
    if (audio->rate<FMN_NULLAUDIO_RATE_MIN) audio->rate=FMN_NULLAUDIO_RATE_MIN;
    else if (audio->rate>FMN_NULLAUDIO_RATE_MAX) audio->rate=FMN_NULLAUDIO_RATE_MAX;
    if (audio->chanc<FMN_NULLAUDIO_CHANC_MAX) audio->chanc=FMN_NULLAUDIO_CHANC_MIN;
    else if (audio->chanc>FMN_NULLAUDIO_CHANC_MAX) audio->chanc=FMN_NULLAUDIO_CHANC_MAX;
  } else {
    audio->rate=22050;
    audio->chanc=1;
  }
  
  return 0;
}

/* Generate a signal and throw it away.
 * Patch in here if you want to dump all output to a WAV file or something.
 */
 
static int fmn_nullaudio_generate_signal(struct fmn_hw_audio *audio,int samplec) {
  if (!audio->delegate->pcm_out) return 0;
  while (samplec>=FMN_NULLAUDIO_BUFFER_SIZE_SAMPLES) {
    audio->delegate->pcm_out(AUDIO->buf,FMN_NULLAUDIO_BUFFER_SIZE_SAMPLES,audio);
    samplec-=FMN_NULLAUDIO_BUFFER_SIZE_SAMPLES;
  }
  if (samplec>0) {
    audio->delegate->pcm_out(AUDIO->buf,samplec,audio);
  }
  return 0;
}

/* Update.
 */
 
static int _nullaudio_update(struct fmn_hw_audio *audio) {
  if (!AUDIO->lasttime) {
    AUDIO->lasttime=fmn_now_s();
    return 0;
  }
  double now=fmn_now_s();
  double elapseds=now-AUDIO->lasttime;
  int elapsedframes=elapseds*audio->rate;
  if (elapsedframes<=0) return 0;
  
  // Advance lasttime by an amount freshly calculated from (elapsedframes). Don't just set it to (now).
  // This mitigates rounding error, for low rates.
  AUDIO->lasttime+=(double)elapsedframes/(double)audio->rate;
  
  if (elapsedframes>FMN_NULLAUDIO_SANITY_LIMIT_FRAMES) elapsedframes=FMN_NULLAUDIO_SANITY_LIMIT_FRAMES;
  fmn_nullaudio_generate_signal(audio,elapsedframes*audio->chanc);
  return 0;
}

/* Type definition.
 */
 
const struct fmn_hw_audio_type fmn_hw_audio_type_nullaudio={
  .name="nullaudio",
  .desc="Dummy driver that generates but discards output.",
  .objlen=sizeof(struct fmn_hw_audio_nullaudio),
  .del=_nullaudio_del,
  .init=_nullaudio_init,
  .update=_nullaudio_update,
};
