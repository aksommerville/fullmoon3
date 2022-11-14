#include "waves_internal.h"

/* Name of C object.
 * We don't bother validating syntax.
 * If it's not a C identifier, the compiler will choke and report it.
 */
 
static int waves_get_c_name(char *dst,int dsta) {

  const char *src=0;
  int srcc=tool_cmdline_get_option(&src,&waves.cmdline,"name",4);
  if (srcc>=0) {
    if (srcc<=dsta) memcpy(dst,src,srcc);
    if (srcc<dsta) dst[srcc]=0;
    return srcc;
  }
  
  const char *path=waves.cmdline.srcpathv[0];
  int ok=1;
  src=path;
  srcc=0;
  for (;*path;path++) {
    if (*path=='/') {
      src=path+1;
      srcc=0;
      ok=1;
    } else if (!ok) {
    } else if (
      ((*path>='a')&&(*path<='z'))||
      ((*path>='A')&&(*path<='Z'))||
      (*path=='_')
    ) {
      srcc++;
    } else {
      ok=0;
    }
  }
  if (!srcc) return -1;
  return snprintf(dst,dsta,"fmnr_waves_%.*s",srcc,src);
}

/* Encode as C text for Cheapsynth.
 */
 
static int waves_encode_c() {
  #define OUT(fmt,...) { if (fmn_encode_fmt(&waves.final,fmt,##__VA_ARGS__)<0) return -1; }
  
  char name[64];
  int namec=waves_get_c_name(name,sizeof(name));
  if ((namec<1)||(namec>=sizeof(name))) {
    fprintf(stderr,"%s: Unable to determine name for C object.\n",waves.cmdline.exename);
    return -2;
  }
  
  OUT("#include <stdint.h>\n")
  if (tool_cmdline_get_option_boolean(&waves.cmdline,"progmem",7)) {
    OUT("#include <avr/pgmspace.h>\n")
  } else {
    OUT("#define PROGMEM\n") // so it needn't be conditional anywhere else
  }
  
  OUT("const int16_t %.*s[] PROGMEM={\n",namec,name)
  int linelen=0;
  const int16_t *v=waves.rawv;
  int i=waves.rawc;
  for (;i-->0;v++) {
    int err=fmn_encode_fmt(&waves.final,"%d,",*v);
    if (err<0) return -1;
    linelen+=err;
    if (linelen>=100) {
      OUT("\n")
      linelen=0;
    }
  }
  if (linelen) OUT("\n")
  OUT("};\n")
  
  OUT("const uint8_t %.*s_count=%d;\n",namec,name,waves.rawc/WAVE_LENGTH);

  #undef OUT
  return 0;
}

/* Encode to C from web text.
 */

static int waves_encode_web() {
  #define OUT(fmt,...) { if (fmn_encode_fmt(&waves.final,fmt,##__VA_ARGS__)<0) return -1; }
  
  char name[64];
  int namec=waves_get_c_name(name,sizeof(name));
  if ((namec<1)||(namec>=sizeof(name))) {
    fprintf(stderr,"%s: Unable to determine name for C object.\n",waves.cmdline.exename);
    return -2;
  }
  
  OUT("#include <stdint.h>\n")
  if (tool_cmdline_get_option_boolean(&waves.cmdline,"progmem",7)) {
    OUT("#include <avr/pgmspace.h>\n")
  } else {
    OUT("#define PROGMEM\n") // so it needn't be conditional anywhere else
  }
  
  OUT("const char %.*s[] PROGMEM=\n",namec,name)
  const char *src=waves.webtext.v;
  int srcp=0;
  while (srcp<waves.webtext.c) {
    OUT("\"")
    while (srcp<waves.webtext.c) {
      char ch=((char*)(waves.webtext.v))[srcp++];
      if (ch==0x0a) OUT("\\n")
      else if ((ch=='"')||(ch=='\\')) OUT("\\%c",ch)
      else if ((ch>=0x20)&&(ch<=0x7e)) OUT("%c",ch)
      else OUT("\\x%02x",(unsigned char)ch)
      if (ch==0x0a) break;
    }
    OUT("\"\n")
  }
  OUT(";\n")

  #undef OUT
  return 0;
}

/* Trivial encodings: bin
 */
 
static int waves_encode_bin() {
  if (fmn_encode_raw(&waves.final,waves.rawv,waves.rawc<<1)<0) return -1;
  return 0;
}

/* Generate output, main entry point.
 */
 
int waves_generate_output() {
  
  const char *encoding=0;
  int encodingc=tool_cmdline_get_option(&encoding,&waves.cmdline,"encoding",8);
  if (encodingc<0) { encodingc=1; encoding="c"; }
  
  if ((encodingc==1)&&!memcmp(encoding,"c",1)) return waves_encode_c();
  if ((encodingc==3)&&!memcmp(encoding,"bin",3)) return waves_encode_bin();
  if ((encodingc==3)&&!memcmp(encoding,"web",3)) return waves_encode_web();
  
  fprintf(stderr,
    "%s: Unknown encoding '%.*s'. Expected 'c' or 'bin'.\n",
    waves.cmdline.exename,encodingc,encoding
  );
  return -2;
}
