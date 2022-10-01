#ifndef FMN_ALSA_INTERNAL_H
#define FMN_ALSA_INTERNAL_H

#include "opt/hw/fmn_hw.h"
#include <stdio.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#define FMN_ALSA_RATE_MIN 200
#define FMN_ALSA_RATE_MAX 200000
#define FMN_ALSA_CHANC_MIN 1
#define FMN_ALSA_CHANC_MAX 2
#define FMN_ALSA_DEFAULT_DEVICE "default"

struct fmn_hw_audio_alsa {
  struct fmn_hw_audio hdr;
  snd_pcm_t *alsa;
  int16_t *buf;
  int bufa;
  snd_pcm_uframes_t bufa_frames;
  pthread_t iothd;
  pthread_mutex_t iomtx;
  int ioabort;
};

#define AUDIO ((struct fmn_hw_audio_alsa*)audio)

#endif
