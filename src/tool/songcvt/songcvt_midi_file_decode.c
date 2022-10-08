#include "songcvt_internal.h"

#define MIDI_CHUNK_MThd 0x4d546864
#define MIDI_CHUNK_MTrk 0x4d54726b

/* MThd.
 */
 
static int songcvt_midi_file_receive_MThd(struct songcvt_midi_file *file,const uint8_t *src,int srcc) {
  if (file->division) {
    fprintf(stderr,"%s: Multiple MThd\n",file->path);
    return -2;
  }
  if (srcc<6) {
    fprintf(stderr,"%s: Invalid MThd length %d, expected 6\n",file->path,srcc);
    return -2;
  }
  file->format=(src[0]<<8)|src[1];
  file->track_count=(src[2]<<8)|src[3];
  file->division=(src[4]<<8)|src[5];
  
  if (file->format!=1) {
    fprintf(stderr,"%s:WARNING: Format %d, expected 1. Will try anyway.\n",file->path,file->format);
  }
  if (!file->division) {
    fprintf(stderr,"%s: Invalid MThd.division==0\n",file->path);
    return -2;
  }
  if (file->division&0x8000) {
    fprintf(stderr,"%s: SMPTE timing not supported\n",file->path);
    return -2;
  }
  return 0;
}

/* MTrk.
 */
 
static int songcvt_midi_file_receive_MTrk(struct songcvt_midi_file *file,const uint8_t *src,int srcc) {
  if (file->trackc>=file->tracka) {
    int na=file->tracka+8;
    if (na>INT_MAX/sizeof(struct songcvt_midi_track)) return -1;
    void *nv=realloc(file->trackv,sizeof(struct songcvt_midi_track)*na);
    if (!nv) return -1;
    file->trackv=nv;
    file->tracka=na;
  }
  struct songcvt_midi_track *track=file->trackv+file->trackc++;
  memset(track,0,sizeof(struct songcvt_midi_track));
  track->v=src;
  track->c=srcc;
  track->delay=-1;
  return 0;
}

/* Dechunk.
 */
 
int songcvt_midi_file_dechunk(struct songcvt_midi_file *file) {
  int srcp=0,err;
  while (1) {
    if (srcp>file->srcc-8) break;
    int srcp0=srcp;
    const uint8_t *P=file->src+srcp;
    const uint32_t chunkid=(P[0]<<24)|(P[1]<<16)|(P[2]<<8)|P[3];
    const int32_t len=(P[4]<<24)|(P[5]<<16)|(P[6]<<8)|P[7];
    srcp+=8;
    if ((len<0)||(srcp>file->srcc-len)) {
      fprintf(stderr,
        "%s:%d/%d: Invalid chunk header 0x%08x,0x%08x\n",
        file->path,srcp0,file->srcc,chunkid,len
      );
      return -2;
    }
    const uint8_t *chunk=file->src+srcp;
    srcp+=len;
    switch (chunkid) {
      case MIDI_CHUNK_MThd: if ((err=songcvt_midi_file_receive_MThd(file,chunk,len))<0) return err; break;
      case MIDI_CHUNK_MTrk: if ((err=songcvt_midi_file_receive_MTrk(file,chunk,len))<0) return err; break;
      default: fprintf(stderr,"%s:%d/%d: Ignoring unknown chunk 0x%08x\n",file->path,srcp0,file->srcc,chunkid);
    }
  }
  if (!file->division) {
    fprintf(stderr,"%s: No MThd chunk\n",file->path);
    return -2;
  }
  return 0;
}

/* Read an event on one track and add it to our list.
 */
 
static int songcvt_midi_track_consume_event(struct songcvt_midi_file *file,struct songcvt_midi_track *track) {
  if (track->p>=track->c) return -1;
  
  if (file->eventc>=file->eventa) {
    int na=file->eventa+1024;
    if (na>INT_MAX/sizeof(struct songcvt_midi_event)) return -1;
    void *nv=realloc(file->eventv,sizeof(struct songcvt_midi_event)*na);
    if (!nv) return -1;
    file->eventv=nv;
    file->eventa=na;
  }
  struct songcvt_midi_event *event=file->eventv+file->eventc++;
  memset(event,0,sizeof(struct songcvt_midi_event));
  
  track->delay=-1;
  event->time=file->time;
  event->trackid=track-file->trackv;
  event->srcp=(track->v-file->src)+track->p;
  
  uint8_t lead;
  if (track->v[track->p]&0x80) lead=track->v[track->p++];
  else if (track->status) lead=track->status;
  else {
    fprintf(stderr,
      "%s:%d/%d: Unexpected leading byte 0x%02x\n",
      file->path,event->srcp,file->srcc,track->v[track->p]
    );
    return -2;
  }
  
  // Initially guess it's a MIDI event, and not Meta or Sysex.
  track->status=lead;
  event->chid=lead&0x0f;
  event->opcode=lead&0xf0;
  
  #define A { if (track->p>track->c-1) return -1; event->a=track->v[track->p++]; }
  #define AB { if (track->p>track->c-2) return -1; event->a=track->v[track->p++]; event->b=track->v[track->p++]; }
  switch (event->opcode) {
    case 0x80: AB break;
    case 0x90: AB if (event->b==0x00) { event->opcode=0x80; event->b=0x40; } break;
    case 0xa0: AB break;
    case 0xb0: AB break;
    case 0xc0: A break;
    case 0xd0: A break;
    case 0xe0: AB break;
    default: {
        track->status=0;
        event->opcode=lead;
        event->chid=0xff;
        switch (event->opcode) {
          case 0xff: A // pass. Meta is the same as Sysex, just with a one-byte "type" first.
          case 0xf0: case 0xf7: {
              int err,len;
              if ((err=fmn_vlq_decode(&len,track->v+track->p,track->c-track->p))<1) return -1;
              track->p+=err;
              if (track->p>track->c-len) return -1;
              event->v=track->v+track->p;
              event->c=len;
              track->p+=len;
            } break;
          default: {
              fprintf(stderr,
                "%s:%d/%d: Unexpected leading byte 0x%02x\n",
                file->path,event->srcp,file->srcc,lead
              );
              return -2;
            }
        }
      } break;
  }
  #undef A
  #undef AB
  return 0;
}

/* Read delay on one track.
 */
 
static int songcvt_midi_track_require_delay(struct songcvt_midi_file *file,struct songcvt_midi_track *track) {
  int err;
  if (track->term) return 0;
  if (track->p>=track->c) {
    track->term=1;
    return 0;
  }
  if (track->delay<0) {
    if ((err=fmn_vlq_decode(&track->delay,track->v+track->p,track->c-track->p))<0) {
      fprintf(stderr,
        "%s:%d/%d:WARNING: Error decoding VLQ. Ignoring remainder of track.\n",
        file->path,(track->v-file->src)+track->p,file->srcc
      );
      track->term=1;
      return 0;
    }
    track->p+=err;
    if (track->p>=track->c) {
      // I believe this is an error; delay must be followed by event. But whatever.
      track->term=1;
      return 0;
    }
  }
  return 0;
}

/* Exhaust all time-zero events.
 */
 
static int songcvt_midi_file_consume_events(struct songcvt_midi_file *file) {
  int err;
  struct songcvt_midi_track *track=file->trackv;
  int i=file->trackc;
  for (;i-->0;track++) {
    while (1) {
      if ((err=songcvt_midi_track_require_delay(file,track))<0) return err;
      if (track->term) break;
      if (track->delay) break;
      if ((err=songcvt_midi_track_consume_event(file,track))<0) return err;
    }
  }
  return 0;
}

/* Read delay on any track that needs it.
 * Returns the smallest pending delay, possibly zero.
 * Or INT_MAX at end of song, or <0 for errors.
 */
 
static int songcvt_midi_file_require_delays(struct songcvt_midi_file *file) {
  int err,mindelay=INT_MAX;
  struct songcvt_midi_track *track=file->trackv;
  int i=file->trackc;
  for (;i-->0;track++) {
    if ((err=songcvt_midi_track_require_delay(file,track))<0) return err;
    if (track->term) continue;
    if (track->delay<mindelay) mindelay=track->delay;
  }
  return mindelay;
}

/* Advance the global clock and update all track clocks.
 */
 
static int songcvt_midi_file_apply_delay(struct songcvt_midi_file *file,int delay) {
  int err;
  struct songcvt_midi_track *track=file->trackv;
  int i=file->trackc;
  for (;i-->0;track++) {
    if ((err=songcvt_midi_track_require_delay(file,track))<0) return err;
    if (track->term) continue;
    if (track->delay<delay) track->delay=0; // oops
    else track->delay-=delay;
  }
  file->time+=delay;
  return 0;
}

/* Read tempo from Meta Set Tempo event.
 */
 
static void songcvt_midi_file_read_tempo(struct songcvt_midi_file *file,const uint8_t *src,int srcc) {
  if (srcc!=3) return;
  file->tempo=(src[0]<<16)|(src[1]<<8)|src[2];
  if (!file->tempo) file->tempo=1;
}

/* Read events.
 */
 
int songcvt_midi_file_read_events(struct songcvt_midi_file *file) {
  int err;
  while (1) {
    int delay=songcvt_midi_file_require_delays(file);
    if (delay<0) return delay;
    if (!delay) {
      if ((err=songcvt_midi_file_consume_events(file))<0) return err;
    } else if (delay==INT_MAX) {
      break;
    } else {
      songcvt_midi_file_apply_delay(file,delay);
    }
  }
  
  // Light initial processing...
  file->tempo=500000;
  struct songcvt_midi_event *event=file->eventv;
  int i=file->eventc;
  for (;i-->0;event++) {
    switch (event->opcode) {
      case 0xff: switch (event->a) {
          case 0x51: songcvt_midi_file_read_tempo(file,event->v,event->c); break;
        } break;
    }
  }
  
  return 0;
}

/* Associate events.
 */
 
int songcvt_midi_file_associate_events(struct songcvt_midi_file *file) {
  struct songcvt_midi_event *eventa=file->eventv;
  int ai=file->eventc;
  for (;ai-->0;eventa++) {
    
    // If we find a Note Off with partner unset, no need to search, we know there was no Note On for it.
    if (eventa->opcode==0x80) {
      if (!eventa->partner) {
        fprintf(stderr,
          "%s:%d/%d:WARNING: Ignoring Note Off track=%d chan=%d note=0x%02x, no matching Note On.\n",
          file->path,eventa->srcp,file->srcc,
          eventa->trackid,eventa->chid,eventa->a
        );
        eventa->opcode=0;
      }
      continue;
    }
    
    // For Note On, find the next Note Off for the same track, channel, and note, which doesn't have a partner yet.
    // "Same track" is a rule I'm imposing; not sure what the spec says about it.
    if (eventa->opcode!=0x90) continue;
    struct songcvt_midi_event *eventb=eventa+1;
    int bi=ai-1;
    for (;bi-->0;eventb++) {
      if (eventb->opcode!=0x80) continue;
      if (eventb->partner) continue;
      if (eventb->trackid!=eventa->trackid) continue;
      if (eventb->chid!=eventa->chid) continue;
      if (eventb->a!=eventa->a) continue;
      
      eventa->partner=eventb;
      eventb->partner=eventa;
      break;
    }
    
    // If we didn't find a Note On's partner, it's illegal but OK: We'll treat it as "instant-off".
    if (!eventa->partner) {
      fprintf(stderr,
        "%s:%d/%d:WARNING: Note On track=%d chan=%d note=0x%02x with no Note Off, will release immediately.\n",
        file->path,eventa->srcp,file->srcc,
        eventa->trackid,eventa->chid,eventa->a
      );
    }
  }
  return 0;
}
