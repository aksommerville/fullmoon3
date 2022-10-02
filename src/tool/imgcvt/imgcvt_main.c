#include "imgcvt_internal.h"

struct imgcvt imgcvt={0};

/* --help
 */
 
static void imgcvt_print_help(const char *exename) {
  fprintf(stderr,"Usage: %s -oOUTPUT -iINPUT [OPTIONS]\n",exename);
  fprintf(stderr,
    "OPTIONS:\n"
    "  --help                     Print this message.\n"
    "  -iPATH,--in=PATH           Input file (PNG).\n"
    "  -oPATH,--out=PATH          Output file.\n"
    "  --fmt=auto|RGBA|Y2|Y8      [auto] Output pixel format.\n"
    "  --alpha=auto|always|never  [auto] Use transparency.\n"
    "  --encoding=c|bin           [c] Output file format. 'bin' is headerless pixels.\n"
    "  --progmem=0|1              [0] Use Arduino PROGMEM declaration.\n"
    "  --name=STRING              Name of C object. Inferred from path if empty.\n"
  );
}

/* Main.
 */

int main(int argc,char **argv) {
  int err;
  imgcvt.cmdline.print_help=imgcvt_print_help;
  imgcvt.cmdline.exename="imgcvt";
  if (
    ((err=tool_cmdline_argv(&imgcvt.cmdline,argc,argv))<0)||
    ((err=tool_cmdline_one_input(&imgcvt.cmdline))<0)||
    ((err=tool_cmdline_one_output(&imgcvt.cmdline))<0)||
  0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error reading command line.\n",imgcvt.cmdline.exename);
    return 1;
  }

  void *serial=0;
  int serialc=fmn_file_read(&serial,imgcvt.cmdline.srcpathv[0]);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",imgcvt.cmdline.srcpathv[0]);
    return -2;
  }
  
  if ((err=fmn_png_decode(&imgcvt.image,serial,serialc,imgcvt.cmdline.srcpathv[0]))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error decoding PNG\n",imgcvt.cmdline.srcpathv[0]);
    return -2;
  }
  
  if ((err=imgcvt_reformat_image())<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to reformat image.\n",imgcvt.cmdline.srcpathv[0]);
    return 1;
  }
  
  if ((err=imgcvt_generate_output())<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to reencode image.\n",imgcvt.cmdline.srcpathv[0]);
    return 1;
  }
  
  if (fmn_file_write(imgcvt.cmdline.dstpath,imgcvt.final.v,imgcvt.final.c)<0) {
    fprintf(stderr,"%s: Failed to write file.\n",imgcvt.cmdline.dstpath);
    return 1;
  }
  
  return 0;
}
