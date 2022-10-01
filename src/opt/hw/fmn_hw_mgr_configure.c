#include "fmn_hw_internal.h"
#include "opt/serial/fmn_serial.h"
#include "opt/fs/fmn_fs.h"

/* Configure single field.
 */
 
static int fmn_hw_mgr_configure_kv(struct fmn_hw_mgr *mgr,const char *k,int kc,const char *v,int vc,const char *refname,int lineno) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  if (!refname) refname="<command-line>";
  
  int vn=0,vok=1;
  if (fmn_int_eval(&vn,v,vc)<1) vn=vok=0;
  
  if (mgr->delegate.config) {
    int err=mgr->delegate.config(mgr,k,kc,v,vc,vn);
    if (err>0) return 0;
    if (err<0) {
      if (err!=-2) {
        fprintf(stderr,"%s:%d: Error processing option '%.*s' = '%.*s'\n",refname,lineno,kc,k,vc,v);
      }
      return -2;
    }
  }
  
  #define STRPARAM(K,fldname) if ((kc==sizeof(K)-1)&&!memcmp(k,K,kc)) { \
    if (mgr->fldname) free(mgr->fldname); \
    if (!(mgr->fldname=malloc(vc+1))) return -1; \
    memcpy(mgr->fldname,v,vc); \
    mgr->fldname[vc]=0; \
    return 0; \
  }
  #define INTPARAM(K,fldname,lo,hi) if ((kc==sizeof(K)-1)&&!memcmp(k,K,kc)) { \
    if (!vok) { \
      fprintf(stderr,"%s:%d: Expected integer in %d..%d for '%.*s', found '%.*s'\n",refname,lineno,lo,hi,kc,k,vc,v); \
      return -2; \
    } \
    if ((vn<lo)||(vn>hi)) { \
      fprintf(stderr,"%s:%d: %d out of range %d..%d for '%.*s'\n",refname,lineno,vn,lo,hi,kc,k); \
      return -2; \
    } \
    mgr->fldname=vn; \
    return 0; \
  }
  
  STRPARAM("video",video_names)
  STRPARAM("audio",audio_names)
  STRPARAM("input",input_names)
  STRPARAM("render",render_names)
  STRPARAM("synth",synth_names)
  INTPARAM("audio-rate",audio_params.rate,200,200000)
  INTPARAM("audio-chanc",audio_params.chanc,1,2)
  STRPARAM("audio-device",audio_params.device)
  INTPARAM("fullscreen",video_params.fullscreen,0,1)
  INTPARAM("width",video_params.winw,1,4096)
  INTPARAM("height",video_params.winh,1,4096)
  
  #undef STRPARAM
  #undef INTPARAM
  
  fprintf(stderr,"%s:%d: Unknown option '%.*s' = '%.*s'\n",refname,lineno,kc,k,vc,v);
  return -2;
}

/* Argv.
 */
 
int fmn_hw_mgr_configure_argv(struct fmn_hw_mgr *mgr,int argc,char **argv) {
  if (mgr->ready) return -1;
  int argi=1,err;
  while (argi<argc) {
    const char *arg=argv[argi++];
    
    if (!arg||!arg[0]) continue;
    
    // No leading dash, it's a config file.
    if (arg[0]!='-') {
      if ((err=fmn_hw_mgr_configure_file(mgr,arg,1))<0) return err;
      continue;
    }
    
    // Single dash alone is reserved for future use.
    if (!arg[1]) {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",argv[0],arg);
      return -2;
    }
    
    // Single leading dash is a short option.
    if (arg[1]!='-') {
      const char *v="1";
      if (arg[2]) v=arg+2;
      else if ((argi<argc)&&argv[argi]&&argv[argi][0]&&(argv[argi][0]!='-')) v=argv[argi++];
      if ((err=fmn_hw_mgr_configure_kv(mgr,arg+1,1,v,-1,argv[0],0))<0) {
        if (err!=-2) fprintf(stderr,"%s: Error processing option '%.2s'\n",argv[0],arg);
        return -2;
      }
      continue;
    }
    
    // Two or more dashes are long options, or error if just dashes.
    while (arg[0]=='-') arg++;
    if (!arg[0]) {
      fprintf(stderr,"%s: Unexpected argument '--'\n",argv[0]);
      return -2;
    }
    const char *k=arg;
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    const char *v=0;
    if (k[kc]=='=') v=k+kc+1;
    else if ((argi<argc)&&argv[argi]&&argv[argi][0]&&(argv[argi][0]!='-')) v=argv[argi++];
    if ((err=fmn_hw_mgr_configure_kv(mgr,k,kc,v,-1,argv[0],0))<0) {
      if (err!=-2) fprintf(stderr,"%s: Error processing option '--%.*s=%s'\n",argv[0],kc,k,v);
      return -2;
    }
  }
  return 0;
}

/* File.
 */
 
int fmn_hw_mgr_configure_file(struct fmn_hw_mgr *mgr,const char *path,int require) {
  if (mgr->ready) return -1;
  char *src=0;
  int srcc=fmn_file_read(&src,path);
  if (srcc<0) {
    if (require) {
      fprintf(stderr,"%s: Failed to read file.\n",path);
      return -2;
    }
    return 0;
  }
  int err=fmn_hw_mgr_configure_text(mgr,src,srcc,path);
  free(src);
  return err;
}

/* Text.
 */

int fmn_hw_mgr_configure_text(struct fmn_hw_mgr *mgr,const char *src,int srcc,const char *refname) {
  if (mgr->ready) return -1;
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0,lineno=0;
  while (srcp<srcc) {
  
    lineno++;
    const char *line=src+srcp;
    int linec=0,comment=0;
    while (srcp<srcc) {
      if (src[srcp]==0x0a) { srcp++; break; }
      if (src[srcp]=='#') srcp++;
      else if (comment) srcp++;
      else { srcp++; linec++; }
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    
    int sepp=-1;
    int i=0; for (;i<linec;i++) if (line[i]=='=') {
      sepp=i;
      break;
    }
    const char *k=line,*v;
    int kc,vc;
    if (sepp<0) {
      kc=linec;
      v="1";
      vc=1;
    } else {
      kc=sepp;
      v=line+sepp+1;
      vc=linec-sepp-1;
    }
    
    int err=fmn_hw_mgr_configure_kv(mgr,k,kc,v,vc,refname,lineno);
    if (err<0) {
      if (refname&&(err!=-2)) fprintf(stderr,"%s:%d: Error processing option '%.*s' = '%.*s'\n",refname,lineno,kc,k,vc,v);
      return -2;
    }
  }
  return 0;
}

/* Initialize video and render.
 */
 
static int fmn_hw_mgr_init_video_type(struct fmn_hw_mgr *mgr,const struct fmn_hw_video_type *type) {
  if (!(mgr->video=fmn_hw_video_new(type,&mgr->delegate,&mgr->video_params))) return 0;
  return 1;
}

static int fmn_hw_mgr_init_video_name(const char *name,int namec,void *userdata) {
  struct fmn_hw_mgr *mgr=userdata;
  const struct fmn_hw_video_type *type=fmn_hw_video_type_by_name(name,namec);
  if (!type) {
    fprintf(stderr,"Video driver '%.*s' not found.\n",namec,name);
    return 0;
  }
  return fmn_hw_mgr_init_video_type(mgr,type);
}
 
static int fmn_hw_mgr_init_render_type(struct fmn_hw_mgr *mgr,const struct fmn_hw_render_type *type) {
  if (!(mgr->render=fmn_hw_render_new(type,&mgr->delegate,&mgr->render_params))) return 0;
  return 1;
}

static int fmn_hw_mgr_init_render_name(const char *name,int namec,void *userdata) {
  struct fmn_hw_mgr *mgr=userdata;
  const struct fmn_hw_render_type *type=fmn_hw_render_type_by_name(name,namec);
  if (!type) {
    fprintf(stderr,"Renderer '%.*s' not found.\n",namec,name);
    return 0;
  }
  return fmn_hw_mgr_init_render_type(mgr,type);
}
 
static int fmn_hw_mgr_init_video(struct fmn_hw_mgr *mgr) {
  int err;
  
  if (mgr->render_params.tilesize<1) mgr->render_params.tilesize=8;
  mgr->render_params.fbw=FMN_COLC*mgr->render_params.tilesize;
  mgr->render_params.fbh=FMN_ROWC*mgr->render_params.tilesize;
  mgr->video_params.title="Full Moon";
  mgr->video_params.iconrgba=0;//TODO app icon
  mgr->video_params.iconw=0;
  mgr->video_params.iconh=0;
  if ((mgr->video_params.winw<1)||(mgr->video_params.winh<1)) {
    mgr->video_params.winw=mgr->render_params.fbw;
    mgr->video_params.winh=mgr->render_params.fbh;
    const int max=1000;
    int scalex=max/mgr->video_params.winw;
    int scaley=max/mgr->video_params.winh;
    int scale=(scalex<scaley)?scalex:scaley;
    if (scale>1) {
      mgr->video_params.winw*=scale;
      mgr->video_params.winh*=scale;
    }
  }
  
  if (mgr->video_names) {
    if ((err=fmn_for_each_comma_string(mgr->video_names,-1,fmn_hw_mgr_init_video_name,mgr))<0) return err;
  } else {
    int i=0;
    const struct fmn_hw_video_type *type;
    for (;type=fmn_hw_video_type_by_index(i);i++) {
      if (type->by_request_only) continue;
      if ((err=fmn_hw_mgr_init_video_type(mgr,type))<0) return err;
      if (mgr->video) break;
    }
  }
  if (!mgr->video) {
    fprintf(stderr,"Failed to initialize any video driver.\n");
    return -2;
  }
  fprintf(stderr,"Using video driver '%s', size=%d,%d\n",mgr->video->type->name,mgr->video->winw,mgr->video->winh);
  
  if (mgr->render_names) {
    if ((err=fmn_for_each_comma_string(mgr->render_names,-1,fmn_hw_mgr_init_render_name,mgr))<0) return err;
  } else {
    int i=0;
    const struct fmn_hw_render_type *type;
    for (;type=fmn_hw_render_type_by_index(i);i++) {
      if (type->by_request_only) continue;
      if ((err=fmn_hw_mgr_init_render_type(mgr,type))<0) return err;
      if (mgr->render) break;
    }
  }
  if (!mgr->render) {
    fprintf(stderr,"Failed to initialize any renderer.\n");
    return -2;
  }
  fprintf(stderr,"Using renderer '%s'\n",mgr->render->type->name);
  
  return 0;
}

/* Initialize audio driver and synthesizer.
 */
 
static int fmn_hw_mgr_init_audio_type(struct fmn_hw_mgr *mgr,const struct fmn_hw_audio_type *type) {
  if (!(mgr->audio=fmn_hw_audio_new(type,&mgr->delegate,&mgr->audio_params))) return 0;
  return 1;
}

static int fmn_hw_mgr_init_audio_name(const char *name,int namec,void *userdata) {
  struct fmn_hw_mgr *mgr=userdata;
  const struct fmn_hw_audio_type *type=fmn_hw_audio_type_by_name(name,namec);
  if (!type) {
    fprintf(stderr,"Audio driver '%.*s' not found.\n",namec,name);
    return 0;
  }
  return fmn_hw_mgr_init_audio_type(mgr,type);
}
 
static int fmn_hw_mgr_init_synth_type(struct fmn_hw_mgr *mgr,const struct fmn_hw_synth_type *type) {
  if (!(mgr->synth=fmn_hw_synth_new(type,&mgr->delegate,&mgr->synth_params))) return 0;
  return 1;
}

static int fmn_hw_mgr_init_synth_name(const char *name,int namec,void *userdata) {
  struct fmn_hw_mgr *mgr=userdata;
  const struct fmn_hw_synth_type *type=fmn_hw_synth_type_by_name(name,namec);
  if (!type) {
    fprintf(stderr,"Synthesizer '%.*s' not found.\n",namec,name);
    return 0;
  }
  return fmn_hw_mgr_init_synth_type(mgr,type);
}
 
static int fmn_hw_mgr_init_audio(struct fmn_hw_mgr *mgr) {
  int err;
  
  if (!mgr->audio_params.rate) mgr->audio_params.rate=44100;
  if (!mgr->audio_params.chanc) mgr->audio_params.chanc=1;
  mgr->delegate.pcm_out=fmn_hw_mgr_pcm_out;
  mgr->delegate.midi_in=fmn_hw_mgr_midi_in;
  
  if (mgr->audio_names) {
    if ((err=fmn_for_each_comma_string(mgr->audio_names,-1,fmn_hw_mgr_init_audio_name,mgr))<0) return err;
  } else {
    int i=0;
    const struct fmn_hw_audio_type *type;
    for (;type=fmn_hw_audio_type_by_index(i);i++) {
      if (type->by_request_only) continue;
      if ((err=fmn_hw_mgr_init_audio_type(mgr,type))<0) return err;
      if (mgr->audio) break;
    }
  }
  if (!mgr->audio) {
    fprintf(stderr,"Failed to initialize any audio driver.\n");
    return -2;
  }
  fprintf(stderr,"Using audio driver '%s', rate=%d, chanc=%d\n",mgr->audio->type->name,mgr->audio->rate,mgr->audio->chanc);
  
  mgr->synth_params.rate=mgr->audio->rate;
  mgr->synth_params.chanc=mgr->audio->chanc;
  
  if (mgr->synth_names) {
    if ((err=fmn_for_each_comma_string(mgr->synth_names,-1,fmn_hw_mgr_init_synth_name,mgr))<0) return err;
  } else {
    int i=0;
    const struct fmn_hw_synth_type *type;
    for (;type=fmn_hw_synth_type_by_index(i);i++) {
      if (type->by_request_only) continue;
      if ((err=fmn_hw_mgr_init_synth_type(mgr,type))<0) return err;
      if (mgr->synth) break;
    }
  }
  if (!mgr->synth) {
    fprintf(stderr,"Failed to initialize any synthesizer.\n");
    return -2;
  }
  fprintf(stderr,"Using synthesizer '%s'\n",mgr->synth->type->name);
  
  return 0;
}

/* Initialize input drivers.
 */
 
static int fmn_hw_mgr_init_input_type(struct fmn_hw_mgr *mgr,const struct fmn_hw_input_type *type) {
  if (mgr->inputc>=mgr->inputa) {
    int na=mgr->inputa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(mgr->inputv,sizeof(void*)*na);
    if (!nv) return -1;
    mgr->inputv=nv;
    mgr->inputa=na;
  }
  if (!(mgr->inputv[mgr->inputc]=fmn_hw_input_new(type,&mgr->delegate))) return 0;
  mgr->inputc++;
  return 0;
}

static int fmn_hw_mgr_init_input_name(const char *name,int namec,void *userdata) {
  struct fmn_hw_mgr *mgr=userdata;
  const struct fmn_hw_input_type *type=fmn_hw_input_type_by_name(name,namec);
  if (!type) {
    fprintf(stderr,"Input driver '%.*s' not found.\n",namec,name);
    return 0;
  }
  return fmn_hw_mgr_init_input_type(mgr,type);
}
 
static int fmn_hw_mgr_init_input(struct fmn_hw_mgr *mgr) {
  int err;
  if (mgr->input_names) {
    if ((err=fmn_for_each_comma_string(mgr->input_names,-1,fmn_hw_mgr_init_input_name,mgr))<0) return err;
  } else {
    int i=0;
    const struct fmn_hw_input_type *type;
    for (;type=fmn_hw_input_type_by_index(i);i++) {
      if (type->by_request_only) continue;
      if ((err=fmn_hw_mgr_init_input_type(mgr,type))<0) return err;
    }
  }
  return 0;
}

/* Ready.
 */
 
int fmn_hw_mgr_ready(struct fmn_hw_mgr *mgr) {
  if (mgr->ready) return 0;
  int err;
  
  if ((err=fmn_hw_mgr_init_video(mgr))<0) return err;
  if ((err=fmn_hw_mgr_init_input(mgr))<0) return err;
  if ((err=fmn_hw_mgr_init_audio(mgr))<0) return err;
  
  mgr->ready=1;
  return 0;
}
