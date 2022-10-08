#ifndef MAPCVT_INTERNAL_H
#define MAPCVT_INTERNAL_H

#include "tool/common/tool_cmdline.h"
#include "opt/fs/fmn_fs.h"
#include "opt/serial/fmn_serial.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

extern struct mapcvt {
  struct tool_cmdline cmdline;
  struct fmn_encoder bin;
  char **refv;
  int refc,refa;
  struct fmn_encoder final;
} mapcvt;

int mapcvt_decode_text(const char *src,int srcc,const char *path); // populates (bin,refv)
int mapcvt_generate_output(); // populates (final)

#endif
