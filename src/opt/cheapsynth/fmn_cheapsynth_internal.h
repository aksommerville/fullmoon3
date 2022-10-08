#ifndef FMN_CHEAPSYNTH_INTERNAL_H
#define FMN_CHEAPSYNTH_INTERNAL_H

#include "fmn_cheapsynth.h"
#include <string.h>
#include <stdio.h>

extern const int16_t fmnr_waves[];
extern const uint8_t fmnr_waves_count;

#define FMN_CHEAPSYNTH_WAVE_SIZE_BITS 9
#define FMN_CHEAPSYNTH_WAVE_SIZE_SAMPLES (1<<FMN_CHEAPSYNTH_WAVE_SIZE_BITS)
#define FMN_CHEAPSYNTH_P_SHIFT (32-FMN_CHEAPSYNTH_WAVE_SIZE_BITS)

#define FMN_CHEAPSYNTH_SQUARE_LIMIT 8
#define FMN_CHEAPSYNTH_VOICE_LIMIT 8
#define FMN_CHEAPSYNTH_PCM_LIMIT 8
#define FMN_CHEAPSYNTH_CHANNEL_COUNT 16

#define FMN_CHEAPSYNTH_CHANNEL_MODE_DISABLE 0
#define FMN_CHEAPSYNTH_CHANNEL_MODE_SQUARE 1
#define FMN_CHEAPSYNTH_CHANNEL_MODE_VOICE 2
#define FMN_CHEAPSYNTH_CHANNEL_MODE_PCM 3

/* (atkvalue,susvalue) must be nonzero and not equal to each other.
 * (atkstep,decstep,rlsstep) must be calculated such that we will hit the exact target value.
 */
struct fmn_cheapsynth_env {
  uint32_t value;
  uint32_t hold;
  uint32_t step;
  uint32_t atkvalue;
  uint32_t decstep;
  uint32_t susvalue;
  uint32_t sustime;
  uint32_t rlsstep;
};

extern struct fmn_cheapsynth {
  uint16_t rate;
  uint8_t chanc;
  
  const uint8_t *song;
  uint16_t songp,songc;
  uint16_t songrate; // ms/tick
  uint32_t songrate_frames; // frames/tick
  uint8_t songstartp;
  uint16_t songloopp;
  int32_t songdelay; // frames
  
  struct fmn_cheapsynth_channel {
    uint8_t mode;
    uint8_t waveid;
    uint16_t atktimemslo;
    uint16_t dectimemslo;
    uint16_t rlstimemslo;
    uint16_t atklevello;
    uint16_t suslevello;
    uint16_t atktimemshi;
    uint16_t dectimemshi;
    uint16_t rlstimemshi;
    uint16_t atklevelhi;
    uint16_t suslevelhi;
  } channelv[FMN_CHEAPSYNTH_CHANNEL_COUNT];
  
  struct fmn_cheapsynth_square {
    uint32_t p;
    uint32_t pd;
    struct fmn_cheapsynth_env env;
    uint8_t chid,noteid;
  } squarev[FMN_CHEAPSYNTH_SQUARE_LIMIT];
  uint8_t squarec;
  
  struct fmn_cheapsynth_voice {
    const int16_t *src; // null if voice unused; list may be sparse
    uint32_t p;
    uint32_t pd;
    struct fmn_cheapsynth_env env;
    uint8_t chid,noteid;
  } voicev[FMN_CHEAPSYNTH_VOICE_LIMIT];
  uint8_t voicec;
  
  struct fmn_cheapsynth_pcm {
    const int16_t *src;
    uint16_t c;
    uint16_t p;
  } pcmv[FMN_CHEAPSYNTH_PCM_LIMIT];
  uint8_t pcmc;
  
} fmn_cheapsynth;

/* Execute any commands pending at time zero, and return frame count to the next event.
 * Never returns less than one.
 */
int32_t fmn_cheapsynth_update_song();
void fmn_cheapsynth_advance_song(int32_t framec);

/* Mono only, and (v) must be zeroed first.
 */
void fmn_cheapsynth_update_signal(int16_t *v,int16_t c);

void fmn_cheapsynth_expand_stereo(int16_t *v,int16_t framec);

void fmn_cheapsynth_env_init(
  struct fmn_cheapsynth_env *env,
  uint16_t atktimems,uint32_t atklevel,
  uint16_t dectimems,uint32_t declevel,
  uint16_t sustimems,
  uint16_t rlstimems
);
void fmn_cheapsynth_env_init_velocity(
  struct fmn_cheapsynth_env *env,
  const struct fmn_cheapsynth_channel *chan,
  uint8_t velocity
);
void fmn_cheapsynth_env_set_sustain(struct fmn_cheapsynth_env *env);
void fmn_cheapsynth_env_set_sustain_ms(struct fmn_cheapsynth_env *env,uint16_t ms);
void fmn_cheapsynth_env_release(struct fmn_cheapsynth_env *env);

void fmn_cheapsynth_reset_channels();
void fmn_cheapsynth_reset_channel(struct fmn_cheapsynth_channel *chan);

#endif
