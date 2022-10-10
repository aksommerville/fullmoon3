#include "songcvt_internal.h"

/* Name of C object.
 * We don't bother validating syntax.
 * If it's not a C identifier, the compiler will choke and report it.
 */
 
static int songcvt_get_c_name(char *dst,int dsta) {

  const char *src=0;
  int srcc=tool_cmdline_get_option(&src,&songcvt.cmdline,"name",4);
  if (srcc>=0) {
    if (srcc<=dsta) memcpy(dst,src,srcc);
    if (srcc<dsta) dst[srcc]=0;
    return srcc;
  }
  
  const char *path=songcvt.cmdline.srcpathv[0];
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
      ((*path>='0')&&(*path<='9'))||
      (*path=='_')
    ) {
      srcc++;
    } else {
      ok=0;
    }
  }
  if (!srcc) return -1;
  return snprintf(dst,dsta,"fmnr_song_%.*s",srcc,src);
}

/* Encode as C text.
 */
 
static int songcvt_encode_c() {
  #define OUT(fmt,...) { if (fmn_encode_fmt(&songcvt.final,fmt,##__VA_ARGS__)<0) return -1; }
  
  char name[64];
  int namec=songcvt_get_c_name(name,sizeof(name));
  if ((namec<1)||(namec>=sizeof(name))) {
    fprintf(stderr,"%s: Unable to determine name for C object.\n",songcvt.cmdline.exename);
    return -2;
  }
  
  OUT("#include <stdint.h>\n")
  if (tool_cmdline_get_option_boolean(&songcvt.cmdline,"progmem",7)) {
    OUT("#include <avr/pgmspace.h>\n")
  } else {
    OUT("#define PROGMEM\n") // so it needn't be conditional anywhere else
  }
  
  OUT("const uint8_t %.*s[] PROGMEM={\n",namec,name)
  int linelen=0;
  const uint8_t *v=songcvt.bin.v;
  int i=songcvt.bin.c;
  for (;i-->0;v++) {
    int err=fmn_encode_fmt(&songcvt.final,"%d,",*v);
    if (err<0) return -1;
    linelen+=err;
    if (linelen>=100) {
      OUT("\n")
      linelen=0;
    }
  }
  if (linelen) OUT("\n")
  OUT("};\n")
  
  OUT("const uint16_t %.*s_length=%d;\n",namec,name,songcvt.bin.c)

  #undef OUT
  return 0;
}

/* "Encode" as raw data.
 */
 
static int songcvt_encode_bin() {
  if (fmn_encode_raw(&songcvt.final,songcvt.bin.v,songcvt.bin.c)<0) return -1;
  return 0;
}

/* Generate output, main entry point.
 */
 
int songcvt_generate_output() {
  
  const char *encoding=0;
  int encodingc=tool_cmdline_get_option(&encoding,&songcvt.cmdline,"encoding",8);
  if (encodingc<0) { encodingc=1; encoding="c"; }
  
  if ((encodingc==1)&&!memcmp(encoding,"c",1)) return songcvt_encode_c();
  if ((encodingc==3)&&!memcmp(encoding,"bin",3)) return songcvt_encode_bin();
  
  fprintf(stderr,
    "%s: Unknown encoding '%.*s'. Expected 'c' or 'bin'.\n",
    songcvt.cmdline.exename,encodingc,encoding
  );
  return -2;
}
