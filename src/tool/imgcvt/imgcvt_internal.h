#ifndef IMGCVT_INTERNAL_H
#define IMGCVT_INTERNAL_H

#include "tool/common/tool_cmdline.h"
#include "opt/fs/fmn_fs.h"
#include "opt/serial/fmn_serial.h"
#include "game/image/fmn_image.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

extern struct imgcvt {
  struct tool_cmdline cmdline;
  struct fmn_image image;
  struct fmn_encoder final;
  
  // tileprops input
  uint8_t tilepropv[256]; // 0,1,2,3; first table only
  
  // binary tileprops, physics only. 64 bytes or empty.
  struct fmn_encoder tilepropsbin;
  
} imgcvt;

int imgcvt_reformat_image(); // overwrites (image)
int imgcvt_read_tileprops(const char *path); // populates (tilepropv,tilepropsbin)
int imgcvt_generate_output(); // populates (final)

#endif
