#include "fmn_cheapsynth_internal.h"

/* Initialize envelope runner.
 */
 
void fmn_cheapsynth_env_init(
  struct fmn_cheapsynth_env *env,
  uint16_t atktimems,uint32_t atklevel,
  uint16_t dectimems,uint32_t declevel,
  uint16_t sustimems,
  uint16_t rlstimems
) {
  env->value=0;
  
  // Start with the leg times. Minimum one frame each.
  uint32_t atkc=((uint32_t)atktimems*fmn_cheapsynth.rate)/1000;
  uint32_t decc=((uint32_t)dectimems*fmn_cheapsynth.rate)/1000;
  uint32_t susc=((uint32_t)sustimems*fmn_cheapsynth.rate)/1000;
  uint32_t rlsc=((uint32_t)rlstimems*fmn_cheapsynth.rate)/1000;
  if (!atkc) atkc=1;
  if (!decc) decc=1;
  if (!susc) susc=1;
  if (!rlsc) rlsc=1;
  
  // (declevel) must be a multiple of (rlsc).
  if (!(env->rlsstep=declevel/rlsc)) env->rlsstep=1;
  declevel=env->rlsstep*rlsc;
  
  // (atklevel) must be greater than (declevel) and their difference a multiple of (decc).
  if (atklevel<declevel+decc) atklevel=declevel+decc;
  if (!(env->decstep=(atklevel-declevel)/decc)) env->decstep=1;
  atklevel=declevel+env->decstep*decc;
  
  // (atkc,atkstep) must be multiples of (atklevel-initial).
  // Cheat initial up to make this true.
  if (!(env->step=atklevel/atkc)) env->step=1;
  env->value=atklevel-env->step*atkc;
  
  env->hold=0;
  env->sustime=susc;
  env->atkvalue=atklevel;
  env->susvalue=declevel;
  env->decstep=-env->decstep;
  env->rlsstep=-env->rlsstep;
}

/* Init envelope with velocity.
 */
 
void fmn_cheapsynth_env_init_velocity(
  struct fmn_cheapsynth_env *env,
  const struct fmn_cheapsynth_channel *chan,
  uint8_t velocity
) {
  if (velocity<=0) {
    fmn_cheapsynth_env_init(env,
      chan->atktimemslo,chan->atklevello,
      chan->dectimemslo,chan->suslevello,
      0,chan->rlstimemslo
    );
  } else if (velocity>=0x7f) {
    fmn_cheapsynth_env_init(env,
      chan->atktimemshi,chan->atklevelhi,
      chan->dectimemshi,chan->suslevelhi,
      0,chan->rlstimemshi
    );
  } else {
    uint8_t lo=0x7f-velocity;
    uint16_t atktime=((uint32_t)chan->atktimemslo*lo+(uint32_t)chan->atktimemshi*velocity)>>7;
    uint16_t dectime=((uint32_t)chan->dectimemslo*lo+(uint32_t)chan->dectimemshi*velocity)>>7;
    uint16_t rlstime=((uint32_t)chan->rlstimemslo*lo+(uint32_t)chan->rlstimemshi*velocity)>>7;
    uint32_t atklevel=((uint32_t)chan->atklevello*lo+(uint32_t)chan->atklevelhi*velocity)<<9;
    uint32_t suslevel=((uint32_t)chan->suslevello*lo+(uint32_t)chan->suslevelhi*velocity)<<9;
    fmn_cheapsynth_env_init(env,
      atktime,atklevel,
      dectime,suslevel,
      0,rlstime
    );
  }
}

/* Initialize envelope sustain.
 */
 
void fmn_cheapsynth_env_set_sustain(struct fmn_cheapsynth_env *env) {
  env->sustime=0xffffffff;
}

void fmn_cheapsynth_env_set_sustain_ms(struct fmn_cheapsynth_env *env,uint16_t ms) {
  env->sustime=((uint32_t)ms*fmn_cheapsynth.rate)/1000;
}

/* Release envelope.
 */
 
void fmn_cheapsynth_env_release(struct fmn_cheapsynth_env *env) {
  env->sustime=0;
  env->hold=0;
}

/* Advance envelope runner.
 */
 
static uint32_t fmn_cheapsynth_env_update(struct fmn_cheapsynth_env *env) {
  if (env->hold) env->hold--;
  else if (env->step) {
    env->value+=env->step;
    if (env->value==env->atkvalue) {
      env->step=env->decstep;
    } else if (env->value==env->susvalue) {
      env->step=env->rlsstep;
      env->hold=env->sustime;
    } else if (!env->value) {
      env->step=0;
    }
  }
  return env->value;
}

/* Update a square wave voice.
 */
 
static void fmn_cheapsynth_square_update(int16_t *v,int16_t c,struct fmn_cheapsynth_square *square) {
  for (;c-->0;v++) {
    square->p+=square->pd;
    int32_t env=fmn_cheapsynth_env_update(&square->env);
    if (!env) {
      square->pd=0;
      return;
    }
    int16_t level=env>>17;
    if (square->p&0x80000000) (*v)+=level;
    else (*v)-=level;
  }
}

/* Update a tuned wave.
 */
 
static void fmn_cheapsynth_voice_update(int16_t *v,int16_t c,struct fmn_cheapsynth_voice *voice) {
  for (;c-->0;v++) {
    voice->p+=voice->pd;
    int32_t env=fmn_cheapsynth_env_update(&voice->env);
    if (!env) {
      voice->src=0;
      return;
    }
    int32_t level=voice->src[voice->p>>FMN_CHEAPSYNTH_P_SHIFT];
    level=(level*(env>>17))>>15;
    (*v)+=level;
  }
}

/* Update verbatim playback.
 */
 
static void fmn_cheapsynth_pcm_update(int16_t *v,int16_t c,struct fmn_cheapsynth_pcm *pcm) {
  if (pcm->c>=pcm->p) {
    pcm->p=pcm->c=0;
    pcm->src=0;
    return;
  }
  uint16_t remaining=pcm->c-pcm->p;
  if (c>remaining) c=remaining;
  for (;c-->0;v++) {
    (*v)+=pcm->src[pcm->p++];
  }
}

/* Generate signal without touching song.
 * Buffer is initially zeroed by the caller.
 */
 
void fmn_cheapsynth_update_signal(int16_t *v,int16_t c) {
  {
    struct fmn_cheapsynth_square *square=fmn_cheapsynth.squarev;
    uint8_t i=fmn_cheapsynth.squarec;
    for (;i-->0;square++) {
      if (!square->pd) continue;
      fmn_cheapsynth_square_update(v,c,square);
    }
  }
  {
    struct fmn_cheapsynth_voice *voice=fmn_cheapsynth.voicev;
    uint8_t i=fmn_cheapsynth.voicec;
    for (;i-->0;voice++) {
      if (!voice->src) continue;
      fmn_cheapsynth_voice_update(v,c,voice);
    }
  }
  {
    struct fmn_cheapsynth_pcm *pcm=fmn_cheapsynth.pcmv;
    uint8_t i=fmn_cheapsynth.pcmc;
    for (;i-->0;pcm++) {
      if (!pcm->src) continue;
      fmn_cheapsynth_pcm_update(v,c,pcm);
    }
  }
}

/* Expand stereo.
 */
 
void fmn_cheapsynth_expand_stereo(int16_t *v,int16_t framec) {
  int16_t *dst=v+(framec<<1);
  const int16_t *src=v+framec;
  while (framec-->0) {
    src--;
    *(--dst)=*src;
    *(--dst)=*src;
  }
}
