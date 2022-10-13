#include "songcvt_internal.h"

/* "debug"
 */
 
static void songcvt_debug_file(struct songcvt_midi_file *file) {
  fprintf(stderr,"=== Stats for %s ===\n",file->path);
  fprintf(stderr,"Input length: %d\n",file->srcc);
  fprintf(stderr,"Format: %d\n",file->format);
  fprintf(stderr,"Track count (per MThd): %d\n",file->track_count);
  fprintf(stderr,"Ticks/qnote: %d\n",file->division);
  fprintf(stderr,"MTrk count: %d\n",file->trackc);
  fprintf(stderr,"Event count: %d\n",file->eventc);
  fprintf(stderr,"Duration: %d ticks\n",file->time);
  fprintf(stderr,"Tempo: %d us/qnote\n",file->tempo);
}

static void songcvt_debug_events(struct songcvt_midi_file *file) {
  fprintf(stderr,"=== Begin event dump for %s ===\n",file->path);
  const struct songcvt_midi_event *event=file->eventv;
  int i=file->eventc;
  for (;i-->0;event++) {
    fprintf(stderr,
      "%8d %6d/%d tr=%d ch=0x%02x op=0x%02x a=0x%02x b=0x%02x c=%d\n",
      event->time,event->srcp,file->srcc,
      event->trackid,event->chid,event->opcode,event->a,event->b,event->c
    );
  }
}
 
static int songcvt_adjust_cmd_debug(
  struct songcvt_midi_file *file,
  const char *src,int srcc,
  const char *path,int lineno
) {
  if (!srcc) { src="all"; srcc=3; }
  if ((srcc==3)&&!memcmp(src,"all",3)) {
    songcvt_debug_file(file);
    songcvt_debug_events(file);
  } else if ((srcc==4)&&!memcmp(src,"file",4)) {
    songcvt_debug_file(file);
  } else if ((srcc==6)&&!memcmp(src,"events",6)) {
    songcvt_debug_events(file);
  } else {
    fprintf(stderr,"%s:%d: 'debug' must be followed by nothing or: 'all','file','events'\n",path,lineno);
    return -2;
  }
  return 0;
}

/* "map"
 */
 
static int songcvt_map_eval_field(
  struct songcvt_midi_event *evf,
  struct songcvt_midi_event *evv,
  const char *src,int srcc,
  int side,
  const char *path,int lineno
) {
  const char *k=src;
  int kc=0;
  while ((kc<srcc)&&(k[kc]!='=')) kc++;
  if (kc>=srcc) {
    fprintf(stderr,"%s:%d: Expected '=' in map token\n",path,lineno);
    return -2;
  }
  const char *v=k+kc+1;
  int vc=srcc-kc-1;
  int n;
  
  #define INTFLD(encname,fldname,lo,hi) if ((kc==sizeof(encname)-1)&&!memcmp(k,encname,kc)) { \
    evf->fldname=1; \
    if (fmn_int_eval(&n,v,vc)<2) { \
      fprintf(stderr,"%s:%d: Failed to evaluate '%.*s' as integer for '%.*s'\n",path,lineno,vc,v,kc,k); \
      return -2; \
    } \
    if ((n<lo)||(n>hi)) { \
      fprintf(stderr,"%s:%d: %d out of range (%d..%d) for '%.*s'\n",path,lineno,n,lo,hi,kc,k); \
      return -2; \
    } \
    evv->fldname=n; \
    return 0; \
  }

  INTFLD("track",trackid,0,255)
  INTFLD("chan",chid,0,255)
  INTFLD("opcode",opcode,0,255)
  INTFLD("note",a,0,255)
  INTFLD("a",a,0,255)
  INTFLD("b",b,0,255)
  
  #undef INTFLD
  fprintf(stderr,"%s:%d: Unknown map field '%.*s'\n",path,lineno,kc,k);
  return -2;
}

static int songcvt_midi_event_match(
  const struct songcvt_midi_event *event,
  const struct songcvt_midi_event *f,
  const struct songcvt_midi_event *v
) {
  if (f->trackid&&(event->trackid!=v->trackid)) return 0;
  if (f->chid&&(event->chid!=v->chid)) return 0;
  if (f->opcode&&(event->opcode!=v->opcode)) return 0;
  if (f->a&&(event->a!=v->a)) return 0;
  if (f->b&&(event->b!=v->b)) return 0;
  return 1;
}

static void songcvt_midi_event_change(
  struct songcvt_midi_event *event,
  const struct songcvt_midi_event *f,
  const struct songcvt_midi_event *v
) {
  if (f->trackid) event->trackid=v->trackid;
  if (f->chid) event->chid=v->chid;
  if (f->opcode) event->opcode=v->opcode;
  if (f->a) event->a=v->a;
  if (f->b) event->b=v->b;
}
 
static int songcvt_adjust_cmd_map(
  struct songcvt_midi_file *file,
  const char *src,int srcc,
  const char *path,int lineno
) {
  int err;
  struct songcvt_midi_event
    matchf={0},
    matchv={0},
    changef={0},
    changev={0};
  int side=0; // 0=match 1=change
  int srcp=0; while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; }
    if ((tokenc==2)&&!memcmp(token,"=>",2)) {
      if (side!=0) {
        fprintf(stderr,"%s:%d: Unexpected second '=>'\n",path,lineno);
        return -2;
      }
      side=1;
    } else if (side==0) {
      if ((err=songcvt_map_eval_field(&matchf,&matchv,token,tokenc,side,path,lineno))<0) {
        if (err!=2) fprintf(stderr,"%s:%d: Failed to evaluate map token '%.*s'\n",path,lineno,tokenc,token);
        return -2;
      }
    } else {
      if ((err=songcvt_map_eval_field(&changef,&changev,token,tokenc,side,path,lineno))<0) {
        if (err!=2) fprintf(stderr,"%s:%d: Failed to evaluate map token '%.*s'\n",path,lineno,tokenc,token);
        return -2;
      }
    }
  }
  if (!side) {
    fprintf(stderr,"%s:%d: Expected '=>' followed by changes, for 'map'\n",path,lineno);
    return -2;
  }
  
  struct songcvt_midi_event *event=file->eventv;
  int i=file->eventc;
  for (;i-->0;event++) {
    if (!songcvt_midi_event_match(event,&matchf,&matchv)) continue;
    songcvt_midi_event_change(event,&changef,&changev);
  }
  return 0;
}

/* "end"
 */
 
static int songcvt_adjust_cmd_end(
  struct songcvt_midi_file *file,
  const char *src,int srcc,
  const char *path,int lineno
) {
  int pad;
  if ((fmn_int_eval(&pad,src,srcc)<2)||(pad<0)) {
    fprintf(stderr,"%s:%d: Failed to evaluate '%.*s' as end padding. Hint: It's usually 4.\n",path,lineno,srcc,src);
    return -2;
  }
  if (pad>32) { // arbitrary limit, but need something to make overflow impossible
    fprintf(stderr,"%s:%d: Unreasonably large pad %d\n",path,lineno,pad);
    return -2;
  }
  int lastofftime=0;
  const struct songcvt_midi_event *event=file->eventv+file->eventc-1;
  int i=file->eventc;
  for (;i-->0;event--) {
    if ((event->opcode!=0x80)&&(event->opcode!=0x90)) continue;
    lastofftime=event->time;
    break;
  }
  
  if (pad) {
    int blocksize=pad*file->division;
    int extra=lastofftime%blocksize;
    if (extra) file->time=lastofftime+blocksize-extra;
    else file->time=lastofftime;
  } else {
    file->time=lastofftime;
  }
  return 0;
}

/* "rate"
 */
 
static int songcvt_adjust_cmd_rate(
  struct songcvt_midi_file *file,
  const char *src,int srcc,
  const char *path,int lineno
) {
  int n;
  if ((fmn_int_eval(&n,src,srcc)<2)||(n<0)||(n>255)) {
    // Zero is legal but redundant, it means "auto".
    fprintf(stderr,"%s:%d: Failed to parse '%.*s' as output tick rate, expected 0..255 (ms/tick)\n",path,lineno,srcc,src);
    return -2;
  }
  file->rate=n;
  return 0;
}

/* "tempo"
 */
 
static int songcvt_adjust_cmd_tempo(
  struct songcvt_midi_file *file,
  const char *src,int srcc,
  const char *path,int lineno
) {
  int n;
  if ((fmn_int_eval(&n,src,srcc)<2)||(n<1)||(n>0xffffff)) {
    fprintf(stderr,"%s:%d: Failed to parse '%.*s' as input tempo (us/qnote)\n",path,lineno,srcc,src);
    return -2;
  }
  file->tempo=n;
  return 0;
}

/* "mode"
 */
 
static int songcvt_adjust_cmd_mode(
  struct songcvt_midi_file *file,
  const char *src,int srcc,
  const char *path,int lineno
) {
  const char *token;
  int srcp=0,tokenc,err,n;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((fmn_int_eval(&n,token,tokenc)<2)||(n<0)||(n>=SONGCVT_CHANNEL_COUNT)) {
    fprintf(stderr,
      "%s:%d: Expected channel id 0..%d, found '%.*s'\n",
      path,lineno,SONGCVT_CHANNEL_COUNT-1,tokenc,token
    );
    return -2;
  }
  uint8_t chid=n;
  struct songcvt_channel *channel=songcvt.channelv+chid;
  
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((tokenc==7)&&!memcmp(src,"disable",7)) channel->mode=0;
  else if ((tokenc==6)&&!memcmp(token,"square",6)) channel->mode=1;
  else if ((tokenc==5)&&!memcmp(token,"voice",5)) channel->mode=2;
  else if ((tokenc==3)&&!memcmp(token,"pcm",3)) channel->mode=3;
  else {
    fprintf(stderr,
      "%s:%d: Unknown channel mode '%.*s'. Must be one of: disable,square,voice,pcm\n",
      path,lineno,tokenc,token
    );
    return -2;
  }
  channel->flags|=SONGCVT_CHANNEL_FLAG_MODE;
  
  // (waveid) in "voice" mode.
  if (channel->mode==2) {
    token=src+srcp;
    tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; }
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if ((fmn_int_eval(&n,token,tokenc)<2)||(n<0)||(n>=0xff)) {
      fprintf(stderr,
        "%s:%d: Expected wave id 0..255, found '%.*s'\n",
        path,lineno,tokenc,token
      );
      return -2;
    }
    channel->waveid=n;
  }
  
  if (srcp<srcc) {
    fprintf(stderr,"%s:%d:WARNING: Unexpected tokens at end of 'mode' line\n",path,lineno);
  }
  
  return 0;
}

/* "env"
 */
 
static int songcvt_adjust_cmd_env(
  struct songcvt_midi_file *file,
  const char *src,int srcc,
  const char *path,int lineno
) {
  const char *token;
  int srcp=0,tokenc,err,n;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((fmn_int_eval(&n,token,tokenc)<2)||(n<0)||(n>=SONGCVT_CHANNEL_COUNT)) {
    fprintf(stderr,
      "%s:%d: Expected channel id 0..%d, found '%.*s'\n",
      path,lineno,SONGCVT_CHANNEL_COUNT-1,tokenc,token
    );
    return -2;
  }
  uint8_t chid=n;
  struct songcvt_channel *channel=songcvt.channelv+chid;
  
  struct songcvt_env *env;
  if ((srcp<=srcc-3)&&!memcmp(src+srcp,"lo",2)&&((unsigned char)src[srcp+2]<=0x20)) {
    srcp+=2;
    env=&channel->lo;
    channel->flags|=SONGCVT_CHANNEL_FLAG_ENVLO;
  } else if ((srcp<=srcc-3)&&!memcmp(src+srcp,"hi",2)&&((unsigned char)src[srcp+2]<=0x20)) {
    srcp+=2;
    env=&channel->hi;
    channel->flags|=SONGCVT_CHANNEL_FLAG_ENVHI;
  } else {
    env=&channel->lo;
    channel->flags|=SONGCVT_CHANNEL_FLAG_ENVLO;
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  #define INTFLD(name) { \
    token=src+srcp; \
    tokenc=0; \
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; } \
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++; \
    if ((fmn_int_eval(&n,token,tokenc)<2)||(n<0)||(n>=0xffff)) { \
      fprintf(stderr, \
        "%s:%d: Unexpected value '%.*s' for %s, must be in 0..65535\n", \
        path,lineno,tokenc,token,#name \
      ); \
      return -2; \
    } \
    env->name=n; \
  }
  INTFLD(atktime)
  INTFLD(atklevel)
  INTFLD(dectime)
  INTFLD(suslevel)
  INTFLD(rlstime)
  #undef INTFLD
  
  if (srcp<srcc) {
    fprintf(stderr,"%s:%d:WARNING: Unexpected tokens after env line\n",path,lineno);
  }
    
  return 0;
}

/* Adjust, single command.
 */
 
static int songcvt_midi_file_adjust_line(
  struct songcvt_midi_file *file,
  const char *src,int srcc,
  const char *path,int lineno
) {
  const char *kw=src;
  int kwc=0,srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; kwc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  if ((kwc==5)&&!memcmp(kw,"debug",5)) return songcvt_adjust_cmd_debug(file,src+srcp,srcc-srcp,path,lineno);
  if ((kwc==3)&&!memcmp(kw,"map",3)) return songcvt_adjust_cmd_map(file,src+srcp,srcc-srcp,path,lineno);
  if ((kwc==3)&&!memcmp(kw,"end",3)) return songcvt_adjust_cmd_end(file,src+srcp,srcc-srcp,path,lineno);
  if ((kwc==4)&&!memcmp(kw,"rate",4)) return songcvt_adjust_cmd_rate(file,src+srcp,srcc-srcp,path,lineno);
  if ((kwc==5)&&!memcmp(kw,"tempo",5)) return songcvt_adjust_cmd_tempo(file,src+srcp,srcc-srcp,path,lineno);
  if ((kwc==4)&&!memcmp(kw,"mode",4)) return songcvt_adjust_cmd_mode(file,src+srcp,srcc-srcp,path,lineno);
  if ((kwc==3)&&!memcmp(kw,"env",3)) return songcvt_adjust_cmd_env(file,src+srcp,srcc-srcp,path,lineno);
  
  fprintf(stderr,"%s:%d: Unknown command '%.*s'\n",path,lineno,kwc,kw);
  return -2;
}

/* Adjust, main entry point.
 */
 
int songcvt_midi_file_adjust(struct songcvt_midi_file *file,const char *path,int require) {
  char *src=0;
  int srcc=fmn_file_read(&src,path);
  if (srcc<0) {
    if (!require) return 0;
    fprintf(stderr,"%s: Failed to read file\n",path);
    return -2;
  }
  
  int srcp=0,lineno=0,err;
  while (srcp<srcc) {
  
    lineno++;
    const char *line=src+srcp;
    int linec=0,comment=0;
    while (srcp<srcc) {
      if (src[srcp]==0x0a) { srcp++; break; }
      if (src[srcp]=='#') comment=1;
      else if (!comment) linec++;
      srcp++;
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    
    if ((err=songcvt_midi_file_adjust_line(file,line,linec,path,lineno))<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error applying adjustment.\n",path,lineno);
      free(src);
      return -2;
    }
  }
  
  free(src);
  return 0;
}
