#include "songcvt_internal.h"
#include <math.h>

// A special event opcode that only gets used during reencode, in place of Note Off where we need to emit them.
#define OPCODE_MANUAL_NOTE_OFF 1

/* Choose an output tick rate.
 * TODO There's probably a smart way to do this, to get the optimal balance of accuracy and file size.
 * I'm not sure where to begin with that.
 * So for now locking it in hard at 40 ms/tick. (which is maybe a bit long)
 * We can of course override manually via the adjust file.
 */
 
static int songcvt_guess_rate() {
  songcvt.midi->rate=40;
  return 0;
}

/* Rewrite all event->time as ms, from input ticks.
 * Or output ticks, from ms.
 */
 
static void songcvt_times_ms_from_srcticks() {
  double scale=(double)songcvt.midi->tempo/((double)songcvt.midi->division*1000.0);
  struct songcvt_midi_event *event=songcvt.midi->eventv;
  int i=songcvt.midi->eventc;
  for (;i-->0;event++) {
    event->time=lround(event->time*scale);
  }
  songcvt.midi->time=lround(songcvt.midi->time*scale);
}
 
static void songcvt_times_dstticks_from_ms() {
  double scale=1.0/(double)songcvt.midi->rate;
  struct songcvt_midi_event *event=songcvt.midi->eventv;
  int i=songcvt.midi->eventc;
  for (;i-->0;event++) {
    event->time=lround(event->time*scale);
  }
  songcvt.midi->time=lround(songcvt.midi->time*scale);
}

/* Encode delays and advance the output clock to a given time, in absolute output ticks.
 */
 
static int songcvt_encode_delay_to(struct fmn_encoder *dst,int time) {
  int tickc=time-songcvt.outtime;
  while (tickc>=0x7f) {
    if (fmn_encode_u8(dst,0x7f)<0) return -1;
    tickc-=0x7f;
  }
  if (tickc>0) {
    if (fmn_encode_u8(dst,tickc)<0) return -1;
  }
  songcvt.outtime=time;
  return 0;
}

/* Encode Note On.
 */
 
static int songcvt_encode_note_on(
  struct fmn_encoder *dst,
  struct songcvt_midi_event *event
) {
  if (event->chid>=0x10) return 0;
  if (songcvt_encode_delay_to(dst,event->time)<0) return -1;
  
  int duration=0;
  if (event->partner) duration=event->partner->time-event->time;
  if (duration<0) duration=0;
  
  // If duration and noteid are agreeable, use the one-shot Note command.
  if ((duration<0x40)&&(event->a>=0x20)&&(event->a<0x60)) {
    uint8_t serial[]={
      0x80|(event->b>>2),
      ((event->a-0x20)<<2)|(duration>>4),
      (duration<<4)|event->chid,
    };
    if (fmn_encode_raw(dst,serial,sizeof(serial))<0) return -1;
    
  // If there is no partner, that's awkward. Duration must be zero. Emit both Note On and Note Off.
  } else if (!event->partner) {
    uint8_t serial[]={
      0xb0|event->chid,
      event->a,
      event->b,
      0xc0|event->chid,
      event->a,
    };
    if (fmn_encode_raw(dst,serial,sizeof(serial))<0) return -1;
    
  // Too long, too low, or too high. No worries. Encode as Note On, and mark the partner as needing encoded.
  } else {
    uint8_t serial[]={
      0xb0|event->chid,
      event->a,
      event->b,
    };
    if (fmn_encode_raw(dst,serial,sizeof(serial))<0) return -1;
    event->partner->opcode=OPCODE_MANUAL_NOTE_OFF;
  }
  
  return 0;
}

/* Encode Note Off.
 */
 
static int songcvt_encode_note_off(
  struct fmn_encoder *dst,
  struct songcvt_midi_event *event
) {
  if (event->chid>=0x10) return 0;
  if (songcvt_encode_delay_to(dst,event->time)<0) return -1;
  uint8_t serial[]={
    0xc0|event->chid,
    event->a,
  };
  if (fmn_encode_raw(dst,serial,sizeof(serial))<0) return -1;
  return 0;
}

/* Encode one event, if needed.
 */
 
static int songcvt_encode_event(
  struct fmn_encoder *dst,
  struct songcvt_midi_event *event
) {
  switch (event->opcode) {
    case 0x90: return songcvt_encode_note_on(dst,event);
    case OPCODE_MANUAL_NOTE_OFF: return songcvt_encode_note_off(dst,event);
  }
  return 0;
}

/* Encode channel header.
 */
 
static int songcvt_encode_channel_header(
  struct fmn_encoder *dst,
  struct songcvt_channel *channel
) {
  uint8_t chid=channel-songcvt.channelv;
  
  if (channel->flags&SONGCVT_CHANNEL_FLAG_MODE) {
    uint8_t voicetype[]={0xa0|chid,0x01,channel->mode};
    if (fmn_encode_raw(dst,voicetype,sizeof(voicetype))<0) return -1;
    if (channel->mode==2) {
      uint8_t setwave[]={0xa0|chid,0x02,channel->waveid};
      if (fmn_encode_raw(dst,setwave,sizeof(setwave))<0) return -1;
    }
  }
  
  if (channel->flags&SONGCVT_CHANNEL_FLAG_ENVLO) {
    if (
      (channel->lo.atktime>0xff)||
      (channel->lo.dectime>0xff)||
      (channel->lo.rlstime>0x7f8)
    ) {
      fprintf(stderr,
        "%s:WARNING: Channel %d env times will overflow. Have (%d,%d,%d), limit (%d,%d,%d).\n",
        songcvt.cmdline.srcpathv[0],chid,
        channel->lo.atktime,channel->lo.dectime,channel->lo.rlstime,
        0xff,0xff,0x7f8
      );
    }
    uint8_t cmd[3]={0xa0|chid};
    cmd[1]=0x03; cmd[2]=channel->lo.atktime; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
    cmd[1]=0x04; cmd[2]=channel->lo.atklevel>>8; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
    cmd[1]=0x05; cmd[2]=channel->lo.dectime; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
    cmd[1]=0x06; cmd[2]=channel->lo.suslevel>>8; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
    cmd[1]=0x07; cmd[2]=channel->lo.rlstime>>3; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
  }
  
  if (channel->flags&SONGCVT_CHANNEL_FLAG_ENVHI) {
    if (
      (channel->hi.atktime>0xff)||
      (channel->hi.dectime>0xff)||
      (channel->hi.rlstime>0x7f8)
    ) {
      fprintf(stderr,
        "%s:WARNING: Channel %d env times will overflow. Have (%d,%d,%d), limit (%d,%d,%d).\n",
        songcvt.cmdline.srcpathv[0],chid,
        channel->hi.atktime,channel->hi.dectime,channel->hi.rlstime,
        0xff,0xff,0x7f8
      );
    }
    uint8_t cmd[3]={0xa0|chid};
    cmd[1]=0x08; cmd[2]=channel->hi.atktime; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
    cmd[1]=0x09; cmd[2]=channel->hi.atklevel>>8; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
    cmd[1]=0x0a; cmd[2]=channel->hi.dectime; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
    cmd[1]=0x0b; cmd[2]=channel->hi.suslevel>>8; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
    cmd[1]=0x0c; cmd[2]=channel->hi.rlstime>>3; if (fmn_encode_raw(dst,cmd,sizeof(cmd))<0) return -1;
  }
  
  return 0;
}

/* Full Moon Song from parsed and adjusted MIDI, main entry point.
 */
 
int songcvt_reencode() {
  int err;
  
  /* First, finalize event times and total time in output ticks.
   */
  songcvt_times_ms_from_srcticks();
  if (!songcvt.midi->rate) {
    if ((err=songcvt_guess_rate())<0) return err;
  }
  songcvt_times_dstticks_from_ms();
  
  /* Emit the fixed header.
   */
  songcvt.bin.c=0;
  uint8_t hdr[]={
    songcvt.midi->rate,
    4,
    0,4, // TODO loop point
  };
  if (fmn_encode_raw(&songcvt.bin,hdr,sizeof(hdr))<0) return -1;
  
  /* Emit channel configurations.
   */
  struct songcvt_channel *channel=songcvt.channelv;
  int i=SONGCVT_CHANNEL_COUNT;
  for (;i-->0;channel++) {
    if ((err=songcvt_encode_channel_header(&songcvt.bin,channel))<0) return err;
  }
  
  /* Update header with new loop point.
   */
  if (songcvt.bin.c>0xffff) {
    // unlikely
    fprintf(stderr,"%s: Header exceeds 64 kB\n",songcvt.cmdline.srcpathv[0]);
    return -2;
  }
  songcvt.bin.v[2]=songcvt.bin.c>>8;
  songcvt.bin.v[3]=songcvt.bin.c;
  
  /* Emit notes.
   */
  songcvt.outtime=0;
  struct songcvt_midi_event *event=songcvt.midi->eventv;
  for (i=songcvt.midi->eventc;i-->0;event++) {
    if (songcvt_encode_event(&songcvt.bin,event)<0) return -1;
  }
  
  // EOF and we're done.
  if (fmn_encode_raw(&songcvt.bin,"\0",1)<0) return -1;
  return 0;
}
