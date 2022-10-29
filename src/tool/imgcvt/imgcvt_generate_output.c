#include "imgcvt_internal.h"

/* Name of C object.
 * We don't bother validating syntax.
 * If it's not a C identifier, the compiler will choke and report it.
 */
 
static int imgcvt_get_c_name(char *dst,int dsta) {

  const char *src=0;
  int srcc=tool_cmdline_get_option(&src,&imgcvt.cmdline,"name",4);
  if (srcc>=0) {
    if (srcc<=dsta) memcpy(dst,src,srcc);
    if (srcc<dsta) dst[srcc]=0;
    return srcc;
  }
  
  const char *path=imgcvt.cmdline.srcpathv[0];
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
  return snprintf(dst,dsta,"fmnr_image_%.*s",srcc,src);
}

/* Encode as C text.
 */
 
static int imgcvt_encode_c() {
  #define OUT(fmt,...) { if (fmn_encode_fmt(&imgcvt.final,fmt,##__VA_ARGS__)<0) return -1; }
  
  char name[64];
  int namec=imgcvt_get_c_name(name,sizeof(name));
  if ((namec<1)||(namec>=sizeof(name))) {
    fprintf(stderr,"%s: Unable to determine name for C object.\n",imgcvt.cmdline.exename);
    return -2;
  }
  
  OUT("#include \"game/image/fmn_image.h\"\n")
  if (tool_cmdline_get_option_boolean(&imgcvt.cmdline,"progmem",7)) {
    OUT("#include <avr/pgmspace.h>\n")
  } else {
    OUT("#define PROGMEM\n") // so it needn't be conditional anywhere else
  }
  
  OUT("static const uint8_t %.*s_storage[] PROGMEM={\n",namec,name)
  int linelen=0;
  const uint8_t *v=imgcvt.image.v;
  int i=imgcvt.image.stride*imgcvt.image.h;
  for (;i-->0;v++) {
    int err=fmn_encode_fmt(&imgcvt.final,"%d,",*v);
    if (err<0) return -1;
    linelen+=err;
    if (linelen>=100) {
      OUT("\n")
      linelen=0;
    }
  }
  if (linelen) OUT("\n")
  OUT("};\n")
  
  if (imgcvt.tilepropsbin.c) {
    OUT("static const uint8_t %.*s_tileprops[] PROGMEM={\n",namec,name)
    for (linelen=0,v=imgcvt.tilepropsbin.v,i=imgcvt.tilepropsbin.c;i-->0;v++) {
      int err=fmn_encode_fmt(&imgcvt.final,"%d,",*v);
      if (err<0) return -1;
      linelen+=err;
      if (linelen>=100) {
        OUT("\n")
        linelen=0;
      }
    }
    if (linelen) OUT("\n")
    OUT("};\n")
  }
  
  OUT("struct fmn_image %.*s={\n",namec,name)
  OUT("  .v=(void*)%.*s_storage,\n",namec,name)
  OUT("  .w=%d,\n",imgcvt.image.w)
  OUT("  .h=%d,\n",imgcvt.image.h)
  OUT("  .stride=%d,\n",imgcvt.image.stride)
  OUT("  .fmt=%d,\n",imgcvt.image.fmt)
  OUT("  .flags=%d,\n",imgcvt.image.flags&(FMN_IMAGE_FLAG_TRANSPARENT))
  OUT("  .refc=0,\n")
  if (imgcvt.tilepropsbin.c) {
    OUT("  .tileprops=%.*s_tileprops,\n",namec,name)
  }
  OUT("};\n")

  #undef OUT
  return 0;
}

/* "Encode" as raw data. Just dump the pixels.
 */
 
static int imgcvt_encode_bin() {
  if (fmn_encode_raw(&imgcvt.final,imgcvt.image.v,imgcvt.image.stride*imgcvt.image.h)<0) return -1;
  return 0;
}

/* Generate output, main entry point.
 */
 
int imgcvt_generate_output() {
  
  const char *encoding=0;
  int encodingc=tool_cmdline_get_option(&encoding,&imgcvt.cmdline,"encoding",8);
  if (encodingc<0) { encodingc=1; encoding="c"; }
  
  if ((encodingc==1)&&!memcmp(encoding,"c",1)) return imgcvt_encode_c();
  if ((encodingc==3)&&!memcmp(encoding,"bin",3)) return imgcvt_encode_bin();
  
  fprintf(stderr,
    "%s: Unknown encoding '%.*s'. Expected 'c' or 'bin'.\n",
    imgcvt.cmdline.exename,encodingc,encoding
  );
  return -2;
}
