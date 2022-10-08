#include "mapcvt_internal.h"

struct mapcvt mapcvt={0};

/* --help
 */
 
static void mapcvt_print_help(const char *exename) {
  fprintf(stderr,"Usage: %s -oOUTPUT -iINPUT [OPTIONS]\n",exename);
  fprintf(stderr,
    "OPTIONS:\n"
    "  --help                     Print this message.\n"
    "  -iPATH,--in=PATH           Input file (text).\n"
    "  -oPATH,--out=PATH          Output file (C).\n"
    "  --progmem=0|1              [0] Use Arduino PROGMEM declaration.\n"
    "  --name=STRING              Name of C object. Inferred from path if empty.\n"
  );
}

/* Main.
 */

int main(int argc,char **argv) {
  int err;
  mapcvt.cmdline.print_help=mapcvt_print_help;
  mapcvt.cmdline.exename="mapcvt";
  if (
    ((err=tool_cmdline_argv(&mapcvt.cmdline,argc,argv))<0)||
    ((err=tool_cmdline_one_input(&mapcvt.cmdline))<0)||
    ((err=tool_cmdline_one_output(&mapcvt.cmdline))<0)||
  0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error reading command line.\n",mapcvt.cmdline.exename);
    return 1;
  }

  void *serial=0;
  int serialc=fmn_file_read(&serial,mapcvt.cmdline.srcpathv[0]);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",mapcvt.cmdline.srcpathv[0]);
    return 1;
  }
  
  if ((err=mapcvt_decode_text(serial,serialc,mapcvt.cmdline.srcpathv[0]))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error decoding map\n",mapcvt.cmdline.srcpathv[0]);
    return 1;
  }
  
  if ((err=mapcvt_generate_output())<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to reencode map.\n",mapcvt.cmdline.srcpathv[0]);
    return 1;
  }
  
  if (fmn_file_write(mapcvt.cmdline.dstpath,mapcvt.final.v,mapcvt.final.c)<0) {
    fprintf(stderr,"%s: Failed to write file.\n",mapcvt.cmdline.dstpath);
    return 1;
  }
  
  return 0;
}
