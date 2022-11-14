#include "waves_internal.h"

/* Finish whatever's in the scratch buffer, if anything.
 */
 
static int waves_compile_finish() {
  if (waves.id<0) return 0;
  if (waves.rawc>waves.rawa-WAVE_LENGTH) {
    int na=waves.rawa+WAVE_LENGTH*8;
    if (na>INT_MAX/sizeof(int16_t)) return -1;
    void *nv=realloc(waves.rawv,sizeof(int16_t)*na);
    if (!nv) return -1;
    waves.rawv=nv;
    waves.rawa=na;
  }
  const float *srcp=waves.scratch;
  int16_t *dstp=waves.rawv+waves.rawc;
  int i=WAVE_LENGTH;
  for (;i-->0;dstp++,srcp++) {
    if ((*srcp<-1.0f)||(*srcp>1.0f)) {
      fprintf(stderr,
        "%s:WARNING: Wave %d out of range (%f)\n",
        waves.cmdline.srcpathv[0],
        waves.id,
        *srcp
      );
    }
    *dstp=(*srcp)*32767.0f;
  }
  waves.rawc+=WAVE_LENGTH;
  waves.id=-1;
  return 0;
}

/* "wave"
 */
 
static int waves_compile_line_wave(const char *src,int srcc,const char *path,int lineno) {

  int err=waves_compile_finish();
  if (err<0) return err;

  int id=0;
  if (fmn_int_eval(&id,src,srcc)<2) {
    fprintf(stderr,"%s:%d: 'wave' must be followed by wave ID\n",path,lineno);
    return -2;
  }
  int expect=waves.rawc/WAVE_LENGTH;
  if (id!=expect) {
    fprintf(stderr,"%s:%d: Expected wave ID %d, found %d\n",path,lineno,expect,id);
    return -2;
  }
  
  waves.id=id;
  waves.web=0;
  float *v=waves.scratch;
  int i=WAVE_LENGTH;
  float p=0.0f;
  float pd=(M_PI*2.0f)/WAVE_LENGTH;
  for (;i-->0;v++,p+=pd) *v=sinf(p);
  
  return 0;
}

/* "noise"
 */
 
static int waves_compile_line_noise(const char *src,int srcc,const char *path,int lineno) {
  if (srcc) {
    fprintf(stderr,"%s:%d: 'noise' takes no parameters\n",path,lineno);
    return -2;
  }
  float *v=waves.scratch;
  int i=WAVE_LENGTH;
  for (;i-->0;v++) *v=((rand()&0xffff)-32768)/32768.0f;
  return 0;
}

/* "harmonics"
 */
 
static int waves_compile_line_harmonics(const char *src,int srcc,const char *path,int lineno) {
  #define coefa 32
  float coefv[coefa];
  int coefc=0,warned_coef_limit=0;
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; }
    int v;
    if ((fmn_int_eval(&v,token,tokenc)<0)||(v<0)||(v>999)) {
      fprintf(stderr,
        "%s:%d: Expected integer in 0..999, found '%.*s'\n",
        path,lineno,tokenc,token
      );
      return -2;
    }
    if (coefc<coefa) {
      coefv[coefc++]=v/999.0f;
    } else if (!warned_coef_limit) {
      warned_coef_limit=1;
      fprintf(stderr,"%s:%d:WARNING: Ignoring some coefficients. Limit %d.\n",path,lineno,coefa);
    }
  }
  #undef coefa
  
  float nv[WAVE_LENGTH]={0};
  int coefi=coefc; while (coefi-->0) {
    if (coefv[coefi]<=0.0f) continue;
    float *dst=nv;
    int srcp=0,step=coefi+1;
    int i=WAVE_LENGTH;
    for (;i-->0;dst++,srcp+=step) {
      if (srcp>=WAVE_LENGTH) srcp-=WAVE_LENGTH;
      (*dst)+=waves.scratch[srcp]*coefv[coefi];
    }
  }
  
  memcpy(waves.scratch,nv,sizeof(waves.scratch));
  return 0;
}

/* "normalize"
 */
 
static int waves_compile_line_normalize(const char *src,int srcc,const char *path,int lineno) {
  
  float target=1.0f;
  if (srcc) {
    int i;
    if ((fmn_int_eval(&i,src,srcc)<2)||(i<0)||(i>999)) {
      fprintf(stderr,"%s:%d: Expected integer in 0..999, found '%.*s'\n",path,lineno,srcc,src);
      return -2;
    }
    target=i/999.0f;
  }
  
  float peak=0.0f;
  float *v=waves.scratch;
  int i=WAVE_LENGTH;
  for (;i-->0;v++) {
    float n=*v;
    if (n<0.0f) n=-n;
    if (n>peak) peak=n;
  }
  if (peak<=0.0f) return 0; // Silence is silence, can't scale it up.
  
  float scale=target/peak;
  for (v=waves.scratch,i=WAVE_LENGTH;i-->0;v++) {
    (*v)*=scale;
  }
  return 0;
}

/* "gain"
 */
 
static int waves_compile_line_gain(const char *src,int srcc,const char *path,int lineno) {
  
  int whole=1,fract=0,clip=-1,gate=-1;
  int srcp=0;
  #define TOKEN(vname,require) { \
    const char *token=src+srcp; \
    int tokenc=0; \
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; } \
    if (tokenc) { \
      if ((fmn_int_eval(&vname,token,tokenc)<2)||(vname<0)||(vname>999)) { \
        fprintf(stderr, \
          "%s:%d: Expected integer in 0..999 for '%s', found '%.*s'\n", \
          path,lineno,#vname,tokenc,token \
        ); \
        return -2; \
      } \
    } else if (require) { \
      fprintf(stderr,"%s:%d: Missing value for '%s'\n",path,lineno,#vname); \
      return -2; \
    } \
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++; \
  }
  TOKEN(whole,1)
  TOKEN(fract,0)
  TOKEN(clip,0)
  TOKEN(gate,0)
  #undef TOKEN
  if (srcp<srcc) {
    fprintf(stderr,"%s:%d: Unexpected tokens after 'gain' command\n",path,lineno);
    return -2;
  }
  
  float fgain=whole+fract/999.0f;
  float fclip=clip/999.0f;
  float fgate=gate/999.0f;
  fprintf(stderr,"%s clip?%s gate?%s gain=%f clip=%f gate=%f\n",__func__,(clip>=0)?"yes":"no",(gate>=0)?"yes":"no",fgain,fclip,fgate);
  float *v=waves.scratch;
  int i=WAVE_LENGTH;
  for (;i-->0;v++) {
    float sample=*v;
    sample*=fgain;
    if (clip>=0) {
      if (sample>=fclip) sample=fclip;
      else if (sample<=-fclip) sample=-fclip;
    }
    if (gate>=0) {
      if (sample>=fgate) ;
      else if (sample<=-fgate) ;
      else sample=0.0f;
    }
    *v=sample;
  }
  return 0;
}

/* "web"
 */
 
static int waves_compile_line_web(const char *src,int srcc,const char *path,int lineno) {
  if (srcc) {
    fprintf(stderr,"%s:%d: Unexpected tokens after 'web'\n",path,lineno);
    return -2;
  }
  if (waves.web) {
    fprintf(stderr,"%s:%d: Already in 'web' mode.\n",path,lineno);
    return -2;
  }
  waves.web=1;
  if (fmn_encode_fmt(&waves.webtext,"wave %d\n",waves.id)<0) return -1;
  return 0;
}

/* Compile one line.
 */
 
static int waves_compile_line(const char *src,int srcc,const char *path,int lineno) {

  const char *kw=src;
  int kwc=0,srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; kwc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  // "wave" alone may appear with no wave in progress.
  if ((kwc==4)&&!memcmp(kw,"wave",4)) return waves_compile_line_wave(src+srcp,srcc-srcp,path,lineno);
  if (waves.id<0) {
    fprintf(stderr,"%s:%d: Expected 'wave ID' before commands.\n",path,lineno);
    return -2;
  }
  
  if (waves.web) {
    // Web mode. It doesn't really compile, we just copy the text verbatim.
    if (fmn_encode_raw(&waves.webtext,src,srcc)<0) return -1;
    if (fmn_encode_raw(&waves.webtext,"\n",1)<0) return -1;
    return 0;
    
  } else {
    // Cheapsynth mode.
    if ((kwc==5)&&!memcmp(kw,"noise",5)) return waves_compile_line_noise(src+srcp,srcc-srcp,path,lineno);
    if ((kwc==9)&&!memcmp(kw,"harmonics",9)) return waves_compile_line_harmonics(src+srcp,srcc-srcp,path,lineno);
    if ((kwc==9)&&!memcmp(kw,"normalize",9)) return waves_compile_line_normalize(src+srcp,srcc-srcp,path,lineno);
    if ((kwc==4)&&!memcmp(kw,"gain",4)) return waves_compile_line_gain(src+srcp,srcc-srcp,path,lineno);
    if ((kwc==3)&&!memcmp(kw,"web",3)) return waves_compile_line_web(src+srcp,srcc-srcp,path,lineno);
  }

  fprintf(stderr,"%s:%d: Unexpected keyword '%.*s'\n",path,lineno,kwc,kw);
  return -2;
}

/* Compile, main entry point.
 */
 
int waves_compile(const char *src,int srcc) {
  int srcp=0,lineno=0,err;
  waves.id=-1;
  while (srcp<srcc) {
  
    lineno++;
    const char *line=src+srcp;
    int linec=0,comment=0;
    while (srcp<srcc) {
      if (src[srcp]==0x0a) { srcp++; break; }
      else if (src[srcp]=='#') comment=1;
      else if (!comment) linec++;
      srcp++;
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    if (!linec) continue;
    
    if ((err=waves_compile_line(line,linec,waves.cmdline.srcpathv[0],lineno))<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error compiling waves\n",waves.cmdline.srcpathv[0],lineno);
      return -2;
    }
  }
  if ((err=waves_compile_finish())<0) return err;
  return 0;
}
