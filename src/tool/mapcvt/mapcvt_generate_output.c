#include "mapcvt_internal.h"

/* Name of C object.
 * We don't bother validating syntax.
 * If it's not a C identifier, the compiler will choke and report it.
 */
 
static int mapcvt_get_c_name(char *dst,int dsta) {

  const char *src=0;
  int srcc=tool_cmdline_get_option(&src,&mapcvt.cmdline,"name",4);
  if (srcc>=0) {
    if (srcc<=dsta) memcpy(dst,src,srcc);
    if (srcc<dsta) dst[srcc]=0;
    return srcc;
  }
  
  const char *path=mapcvt.cmdline.srcpathv[0];
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
  return snprintf(dst,dsta,"fmnr_map_%.*s",srcc,src);
}

/* Encode as C text.
 */
 
int mapcvt_generate_output() {
  #define OUT(fmt,...) { if (fmn_encode_fmt(&mapcvt.final,fmt,##__VA_ARGS__)<0) return -1; }
  
  char name[64];
  int namec=mapcvt_get_c_name(name,sizeof(name));
  if ((namec<1)||(namec>=sizeof(name))) {
    fprintf(stderr,"%s: Unable to determine name for C object.\n",mapcvt.cmdline.exename);
    return -2;
  }
  
  OUT("#include <stdint.h>\n")
  OUT("#include \"game/map/fmn_map.h\"\n")
  if (tool_cmdline_get_option_boolean(&mapcvt.cmdline,"progmem",7)) {
    OUT("#include <avr/pgmspace.h>\n")
  } else {
    OUT("#define PROGMEM\n") // so it needn't be conditional anywhere else
  }
  
  // Object references.
  int i;
  OUT("typedef struct {} unknown_external_t;\n")
  for (i=0;i<mapcvt.refc;i++) OUT("extern unknown_external_t %s;\n",mapcvt.refv[i]);
  OUT("static const void *%.*s_refv[] PROGMEM={\n",namec,name)
  for (i=0;i<mapcvt.refc;i++) OUT("  &%s,\n",mapcvt.refv[i])
  OUT("};\n")
  
  // Raw data.
  OUT("static const uint8_t %.*s_serial[] PROGMEM={\n",namec,name)
  int linelen=0;
  const uint8_t *v=mapcvt.bin.v;
  for (i=mapcvt.bin.c;i-->0;v++) {
    int err=fmn_encode_fmt(&mapcvt.final,"%d,",*v);
    if (err<0) return -1;
    linelen+=err;
    if (linelen>=100) {
      OUT("\n")
      linelen=0;
    }
  }
  if (linelen) OUT("\n")
  OUT("};\n")
  
  // Wrapped up in a struct, for a single point of contact.
  OUT("const struct fmn_map_resource %.*s={\n",namec,name)
  OUT("  .serial=%.*s_serial,\n",namec,name)
  OUT("  .refv=%.*s_refv,\n",namec,name)
  OUT("};\n")

  #undef OUT
  return 0;
}
