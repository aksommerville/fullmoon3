#include "waves_internal.h"

struct waves waves={0};

/* --help
 */
 
static void waves_print_help(const char *exename) {
  fprintf(stderr,"Usage: %s -oOUTPUT -iINPUT [OPTIONS]\n",exename);
  fprintf(stderr,
    "OPTIONS:\n"
    "  --help                     Print this message.\n"
    "  -iPATH,--in=PATH           Input file (text).\n"
    "  -oPATH,--out=PATH          Output file.\n"
    "  --encoding=c|bin|web       [c] Output file format, see below.\n"
    "  --progmem=0|1              [0] Use Arduino PROGMEM declaration.\n"
    "  --name=STRING              Name of C object. Inferred from path if empty.\n"
    "\n"
    "--encoding:\n"
    "  c      C source, for baking into the project. Cheapsynth.\n"
    "  bin    Raw s16 binary, host byte order. Cheapsynth.\n"
    "  web    Same as 'c', but use text for the Web synthesizer.\n"
  );
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err;
  waves.cmdline.print_help=waves_print_help;
  waves.cmdline.exename="waves";
  if (
    ((err=tool_cmdline_argv(&waves.cmdline,argc,argv))<0)||
    ((err=tool_cmdline_one_input(&waves.cmdline))<0)||
    ((err=tool_cmdline_one_output(&waves.cmdline))<0)||
  0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error reading command line.\n",waves.cmdline.exename);
    return 1;
  }

  void *src=0;
  int srcc=fmn_file_read(&src,waves.cmdline.srcpathv[0]);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",waves.cmdline.srcpathv[0]);
    return 1;
  }
  
  if ((err=waves_compile(src,srcc))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error during compilation.\n",waves.cmdline.srcpathv[0]);
    return 1;
  }
  
  if ((err=waves_generate_output())<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to reencode image.\n",waves.cmdline.srcpathv[0]);
    return 1;
  }
  
  if (fmn_file_write(waves.cmdline.dstpath,waves.final.v,waves.final.c)<0) {
    fprintf(stderr,"%s: Failed to write file.\n",waves.cmdline.dstpath);
    return 1;
  }
  return 0;
}
