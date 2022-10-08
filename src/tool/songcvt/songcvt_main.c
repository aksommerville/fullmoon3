#include "songcvt_internal.h"

struct songcvt songcvt={0};

/* --help
 */
 
static void songcvt_print_help(const char *exename) {
  fprintf(stderr,"Usage: %s -oOUTPUT -iINPUT [OPTIONS]\n",exename);
  fprintf(stderr,
    "OPTIONS:\n"
    "  --help                     Print this message.\n"
    "  -iPATH,--in=PATH           Input file (MIDI).\n"
    "  -oPATH,--out=PATH          Output file.\n"
    "  --encoding=c|bin           [c] Output file format.\n"
    "  --progmem=0|1              [0] Use Arduino PROGMEM declaration.\n"
    "  --name=STRING              Name of C object. Inferred from path if empty.\n"
    "  --adjust=PATH              Adjustment file, no error if not found. See etc/doc/song-format.txt\n"
  );
}

/* Main.
 */

int main(int argc,char **argv) {
  int err;
  songcvt.cmdline.print_help=songcvt_print_help;
  songcvt.cmdline.exename="songcvt";
  if (
    ((err=tool_cmdline_argv(&songcvt.cmdline,argc,argv))<0)||
    ((err=tool_cmdline_one_input(&songcvt.cmdline))<0)||
    ((err=tool_cmdline_one_output(&songcvt.cmdline))<0)||
  0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error reading command line.\n",songcvt.cmdline.exename);
    return 1;
  }
  
  if (!(songcvt.midi=songcvt_midi_file_new(songcvt.cmdline.srcpathv[0]))) {
    return 1;
  }
  
  const char *adjustpath=0;
  int adjustpathc=tool_cmdline_get_option(&adjustpath,&songcvt.cmdline,"adjust",6);
  if (adjustpathc>0) {
    if ((err=songcvt_midi_file_adjust(songcvt.midi,adjustpath,0))<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error applying adjustments.\n",adjustpath);
      return 1;
    }
  }
  
  if ((err=songcvt_reencode())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error reencoding song.\n",songcvt.cmdline.srcpathv[0]);
    return 1;
  }
  
  if ((err=songcvt_generate_output())<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to generate output.\n",songcvt.cmdline.srcpathv[0]);
    return 1;
  }
  
  if (fmn_file_write(songcvt.cmdline.dstpath,songcvt.final.v,songcvt.final.c)<0) {
    fprintf(stderr,"%s: Failed to write file.\n",songcvt.cmdline.dstpath);
    return 1;
  }
  
  return 0;
}
