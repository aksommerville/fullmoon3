#include "fmn_cheapsynth_internal.h"
#include <math.h>

/* Globals.
 */

struct fmn_cheapsynth fmn_cheapsynth={0};

static uint32_t fmn_cheapsynth_ratev[128]={
  2125742,2252146,2386065,2527948,2678268,2837526,3006254,3185015,3374406,3575058,3787642,4012867,4251485,
  4504291,4772130,5055896,5356535,5675051,6012507,6370030,6748811,7150117,7575285,8025735,8502970,9008582,
  9544261,10111792,10713070,11350103,12025015,12740059,13497623,14300233,15150569,16051469,17005939,18017165,
  19088521,20223584,21426141,22700205,24050030,25480119,26995246,28600467,30301139,32102938,34011878,36034330,
  38177043,40447168,42852281,45400411,48100060,50960238,53990491,57200933,60602276,64205876,68023757,72068660,
  76354085,80894335,85704563,90800821,96200119,101920476,107980983,114401866,121204555,128411753,136047513,
  144137319,152708170,161788671,171409126,181601643,192400238,203840952,215961966,228803732,242409110,256823506,
  272095026,288274639,305416341,323577341,342818251,363203285,384800477,407681904,431923931,457607465,484818220,
  513647012,544190053,576549277,610832681,647154683,685636503,726406571,769600953,815363807,863847862,915214929,
  969636441,1027294024,1088380105,1153098554,1221665363,1294309365,1371273005,1452813141,1539201906,1630727614,
  1727695724,1830429858,1939272882,2054588048,2176760211,2306197109,2443330725,2588618730,2742546010,2905626283,
  3078403812,3261455229,
};
//TODO recalculate for whatever we decide finally for Tiny's audio rate. Right now it's 11025.
static uint16_t fmn_cheapsynth_ratev_base=22050;

/* Init.
 */
 
void fmn_cheapsynth_init(uint16_t rate,uint8_t chanc) {
  if (rate<200) rate=200;
  if (chanc<1) chanc=1;
  else if (chanc>2) chanc=2;
  
  fmn_cheapsynth.rate=rate;
  fmn_cheapsynth.chanc=chanc;
  
  if (rate!=fmn_cheapsynth_ratev_base) {
    uint32_t *v=fmn_cheapsynth_ratev;
    uint8_t i=0x80;
    for (;i-->0;v++) {
      *v=((*v)*(float)fmn_cheapsynth_ratev_base)/rate;
    }
    fmn_cheapsynth_ratev_base=rate;
  }
  
  fmn_cheapsynth_reset_channels();
}

/* Update.
 */
 
void fmn_cheapsynth_update(int16_t *v,int16_t c) {
  memset(v,0,c<<1);
  if (fmn_cheapsynth.chanc==2) {
    int16_t framec=c>>1;
    while (framec>0) {
      int32_t updframec=fmn_cheapsynth_update_song();
      if (updframec>framec) updframec=framec;
      fmn_cheapsynth_update_signal(v,updframec); // sic updframec, generate mono first
      fmn_cheapsynth_expand_stereo(v,updframec);
      fmn_cheapsynth_advance_song(updframec);
      v+=updframec<<1;
      framec-=updframec;
    }
  } else {
    while (c>0) {
      int32_t updc=fmn_cheapsynth_update_song();
      if (updc>c) updc=c;
      fmn_cheapsynth_update_signal(v,updc);
      fmn_cheapsynth_advance_song(updc);
      v+=updc;
      c-=updc;
    }
  }
  while (fmn_cheapsynth.squarec&&!fmn_cheapsynth.squarev[fmn_cheapsynth.squarec-1].pd) fmn_cheapsynth.squarec--;
  while (fmn_cheapsynth.voicec&&!fmn_cheapsynth.voicev[fmn_cheapsynth.voicec-1].src) fmn_cheapsynth.voicec--;
  while (fmn_cheapsynth.pcmc&&!fmn_cheapsynth.pcmv[fmn_cheapsynth.pcmc-1].src) fmn_cheapsynth.pcmc--;
}

/* Play song.
 */
 
void fmn_cheapsynth_play_song(const void *v,uint16_t c) {
  const uint8_t *V=v;
  
  if (!v||(c<4)) {
   _no_song_:;
    fmn_cheapsynth.song=0;
    fmn_cheapsynth.songp=0;
    fmn_cheapsynth.songc=0;
    return;
  }
  
  uint32_t mspertick=V[0];
  uint8_t songstartp=V[1];
  uint16_t songloopp=(V[2]<<8)|V[3];
  if ((songstartp<4)||(songloopp<songstartp)||(songloopp>c)) goto _no_song_;
  
  fmn_cheapsynth.songrate=mspertick;
  if (fmn_cheapsynth.songrate<1) fmn_cheapsynth.songrate=1;
  fmn_cheapsynth.songrate_frames=(fmn_cheapsynth.songrate*fmn_cheapsynth.rate)/1000;
  if (fmn_cheapsynth.songrate_frames<1) fmn_cheapsynth.songrate_frames=1;
  
  fmn_cheapsynth.song=v;
  fmn_cheapsynth.songp=songstartp;
  fmn_cheapsynth.songc=c;
  fmn_cheapsynth.songstartp=songstartp;
  fmn_cheapsynth.songloopp=songloopp;
  fmn_cheapsynth.songdelay=0;
  
  fmn_cheapsynth_reset_channels();
}

/* Get available voice.
 */
 
static struct fmn_cheapsynth_square *fmn_cheapsynth_square_new() {
  struct fmn_cheapsynth_square *square=fmn_cheapsynth.squarev;
  uint8_t i=fmn_cheapsynth.squarec;
  for (;i-->0;square++) {
    if (!square->pd) return square;
  }
  if (fmn_cheapsynth.squarec<FMN_CHEAPSYNTH_SQUARE_LIMIT) {
    return fmn_cheapsynth.squarev+fmn_cheapsynth.squarec++;
  }
  return 0;
}
 
static struct fmn_cheapsynth_voice *fmn_cheapsynth_voice_new() {
  struct fmn_cheapsynth_voice *voice=fmn_cheapsynth.voicev;
  uint8_t i=fmn_cheapsynth.voicec;
  for (;i-->0;voice++) {
    if (!voice->src) return voice;
  }
  if (fmn_cheapsynth.voicec<FMN_CHEAPSYNTH_VOICE_LIMIT) {
    return fmn_cheapsynth.voicev+fmn_cheapsynth.voicec++;
  }
  return 0;
}

/* Begin square wave voice.
 */
 
static struct fmn_cheapsynth_square *fmn_cheapsynth_square_begin(
  const struct fmn_cheapsynth_channel *chan,uint8_t noteid,uint8_t velocity
) {
  struct fmn_cheapsynth_square *square=fmn_cheapsynth_square_new();
  if (!square) return 0;
  square->p=0;
  square->pd=fmn_cheapsynth_ratev[noteid&0x7f];
  fmn_cheapsynth_env_init_velocity(&square->env,chan,velocity);
  square->chid=square->noteid=0xff;
  return square;
}

/* Begin tuned wave voice.
 */
 
static struct fmn_cheapsynth_voice *fmn_cheapsynth_voice_begin(
  const struct fmn_cheapsynth_channel *chan,uint8_t noteid,uint8_t velocity
) {
  if (chan->waveid>=fmnr_waves_count) return 0;
  struct fmn_cheapsynth_voice *voice=fmn_cheapsynth_voice_new();
  if (!voice) return 0;
  voice->p=0;
  voice->pd=fmn_cheapsynth_ratev[noteid&0x7f];
  voice->src=fmnr_waves+chan->waveid*FMN_CHEAPSYNTH_WAVE_SIZE_SAMPLES;
  fmn_cheapsynth_env_init_velocity(&voice->env,chan,velocity);
  voice->chid=voice->noteid=0xff;
  return voice;
}

/* One-off note.
 */
 
void fmn_cheapsynth_note(uint8_t chid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms) {

  //fprintf(stderr,"TODO %s chid=%d note=%d vel=%d dur=%d\n",__func__,chid,noteid,velocity,duration_ms);
  
  if (chid>=FMN_CHEAPSYNTH_CHANNEL_COUNT) return;
  const struct fmn_cheapsynth_channel *chan=fmn_cheapsynth.channelv+chid;
  switch (chan->mode) {
  
    case FMN_CHEAPSYNTH_CHANNEL_MODE_SQUARE: {
        struct fmn_cheapsynth_square *square=fmn_cheapsynth_square_begin(chan,noteid,velocity);
        if (!square) return;
        fmn_cheapsynth_env_set_sustain_ms(&square->env,duration_ms);
      } break;
      
    case FMN_CHEAPSYNTH_CHANNEL_MODE_VOICE: {
        struct fmn_cheapsynth_voice *voice=fmn_cheapsynth_voice_begin(chan,noteid,velocity);
        if (!voice) return;
        fmn_cheapsynth_env_set_sustain_ms(&voice->env,duration_ms);
      } break;
      
    case FMN_CHEAPSYNTH_CHANNEL_MODE_PCM: {
        //TODO How are PCM channels going to work? I'm not sure we need these.
      } break;
  }
}

/* Note On.
 */
 
void fmn_cheapsynth_note_on(uint8_t chid,uint8_t noteid,uint8_t velocity) {
  //fprintf(stderr,"TODO %s %02x %02x %02x\n",__func__,chid,noteid,velocity);
  if (chid>=FMN_CHEAPSYNTH_CHANNEL_COUNT) return;
  const struct fmn_cheapsynth_channel *chan=fmn_cheapsynth.channelv+chid;
  switch (chan->mode) {
  
    case FMN_CHEAPSYNTH_CHANNEL_MODE_SQUARE: {
        struct fmn_cheapsynth_square *square=fmn_cheapsynth_square_begin(chan,noteid,velocity);
        if (!square) return;
        fmn_cheapsynth_env_set_sustain(&square->env);
        square->chid=chid;
        square->noteid=noteid;
      } break;
      
    case FMN_CHEAPSYNTH_CHANNEL_MODE_VOICE: {
        struct fmn_cheapsynth_voice *voice=fmn_cheapsynth_voice_begin(chan,noteid,velocity);
        if (!voice) return;
        fmn_cheapsynth_env_set_sustain(&voice->env);
        voice->chid=chid;
        voice->noteid=noteid;
      } break;
      
    case FMN_CHEAPSYNTH_CHANNEL_MODE_PCM: {
        //TODO PCM channel
      } break;
  }
}

/* Note Off.
 */
 
void fmn_cheapsynth_note_off(uint8_t chid,uint8_t noteid) {
  //fprintf(stderr,"TODO %s %02x %02x\n",__func__,chid,noteid);
  uint8_t i;
  
  struct fmn_cheapsynth_square *square=fmn_cheapsynth.squarev;
  for (i=fmn_cheapsynth.squarec;i-->0;square++) {
    if ((square->chid!=chid)||(square->noteid!=noteid)) continue;
    fmn_cheapsynth_env_release(&square->env);
    square->chid=square->noteid=0xff;
  }
  
  struct fmn_cheapsynth_voice *voice=fmn_cheapsynth.voicev;
  for (i=fmn_cheapsynth.voicec;i-->0;voice++) {
    if ((voice->chid!=chid)||(voice->noteid!=noteid)) continue;
    fmn_cheapsynth_env_release(&voice->env);
    voice->chid=voice->noteid=0xff;
  }
}

/* Config.
 */
 
void fmn_cheapsynth_config(uint8_t chid,uint8_t k,uint8_t v) {
  if (chid>=FMN_CHEAPSYNTH_CHANNEL_COUNT) return;
  struct fmn_cheapsynth_channel *channel=fmn_cheapsynth.channelv+chid;
  switch (k) {
    case 0x00: fmn_cheapsynth_reset_channel(channel); return;
    case 0x01: channel->mode=v; return;
    case 0x02: channel->waveid=v; return;
    case 0x03: channel->atktimemslo=channel->atktimemshi=v; return;
    case 0x04: channel->atklevello=channel->atklevelhi=v|(v<<8); return;
    case 0x05: channel->dectimemslo=channel->dectimemshi=v; return;
    case 0x06: channel->suslevello=channel->suslevelhi=v|(v<<8); return;
    case 0x07: channel->rlstimemslo=channel->rlstimemshi=v<<3; return;
    case 0x08: channel->atktimemshi=v; return;
    case 0x09: channel->atklevelhi=v|(v<<8); return;
    case 0x0a: channel->dectimemshi=v; return;
    case 0x0b: channel->suslevelhi=v|(v<<8); return;
    case 0x0c: channel->rlstimemshi=v<<3; return;
  }
}

/* Silence.
 */
 
void fmn_cheapsynth_silence() {
  uint8_t i;
  struct fmn_cheapsynth_square *square=fmn_cheapsynth.squarev;
  for (i=fmn_cheapsynth.squarec;i-->0;square++) square->pd=0;
  struct fmn_cheapsynth_voice *voice=fmn_cheapsynth.voicev;
  for (i=fmn_cheapsynth.voicec;i-->0;voice++) voice->src=0;
  struct fmn_cheapsynth_pcm *pcm=fmn_cheapsynth.pcmv;
  for (i=fmn_cheapsynth.pcmc;i-->0;pcm++) pcm->src=0;
}

/* Release notes.
 */
 
void fmn_cheapsynth_release_all() {
  uint8_t i;
  struct fmn_cheapsynth_square *square=fmn_cheapsynth.squarev;
  for (i=fmn_cheapsynth.squarec;i-->0;square++) {
    fmn_cheapsynth_env_release(&square->env);
    square->chid=square->noteid=0xff;
  }
  struct fmn_cheapsynth_voice *voice=fmn_cheapsynth.voicev;
  for (i=fmn_cheapsynth.voicec;i-->0;voice++) {
    fmn_cheapsynth_env_release(&voice->env);
    voice->chid=voice->noteid=0xff;
  }
}
