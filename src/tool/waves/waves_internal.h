#ifndef WAVES_INTERNAL_H
#define WAVES_INTERNAL_H

#include "tool/common/tool_cmdline.h"
#include "opt/fs/fmn_fs.h"
#include "opt/serial/fmn_serial.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>

#define WAVE_LENGTH 512

extern struct waves {
  struct tool_cmdline cmdline;
  int id; // wave in progress if >=0, during compile
  float scratch[WAVE_LENGTH]; // current wave during compile, before quantization
  int16_t *rawv;
  int rawc,rawa;
  struct fmn_encoder final;
} waves;

int waves_compile(const char *src,int srcc); // populates (raw)
int waves_generate_output(); // populates (final)

#endif
