#ifndef FMN_HW_INTERNAL_H
#define FMN_HW_INTERNAL_H

#include "fmn_hw.h"
#include "api/fmn_common.h"
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

struct fmn_hw_mgr {

  struct fmn_hw_delegate delegate;
  
  struct fmn_hw_video_params video_params;
  struct fmn_hw_audio_params audio_params;
  struct fmn_hw_synth_params synth_params;
  char *video_names;
  char *audio_names;
  char *input_names;
  char *synth_names;

  struct fmn_hw_video *video;
  struct fmn_hw_audio *audio;
  struct fmn_hw_synth *synth;
  struct fmn_hw_input **inputv;
  int inputc,inputa;
  
  int ready;
  
};

void fmn_hw_mgr_pcm_out(int16_t *v,int c,struct fmn_hw_audio *audio);
void fmn_hw_mgr_midi_in(struct fmn_hw_audio *audio,int devid,const void *v,int c);

#endif
