#include "fmn_hw.h"
#include <string.h>
#include <limits.h>

/* Type registries.
 */
 
extern const struct fmn_hw_video_type fmn_hw_video_type_nullvideo;
extern const struct fmn_hw_video_type fmn_hw_video_type_glx;
extern const struct fmn_hw_video_type fmn_hw_video_type_x11;
extern const struct fmn_hw_video_type fmn_hw_video_type_drmgx;
extern const struct fmn_hw_video_type fmn_hw_video_type_drmfb;
extern const struct fmn_hw_video_type fmn_hw_video_type_mswm;
extern const struct fmn_hw_video_type fmn_hw_video_type_macwm;
extern const struct fmn_hw_video_type fmn_hw_video_type_ttyvideo;

extern const struct fmn_hw_audio_type fmn_hw_audio_type_nullaudio;
extern const struct fmn_hw_audio_type fmn_hw_audio_type_alsa;
extern const struct fmn_hw_audio_type fmn_hw_audio_type_pulse;
extern const struct fmn_hw_audio_type fmn_hw_audio_type_msaudio;
extern const struct fmn_hw_audio_type fmn_hw_audio_type_macaudio;

extern const struct fmn_hw_input_type fmn_hw_input_type_evdev;
extern const struct fmn_hw_input_type fmn_hw_input_type_mshid;
extern const struct fmn_hw_input_type fmn_hw_input_type_machid;

extern const struct fmn_hw_synth_type fmn_hw_synth_type_nullsynth;
extern const struct fmn_hw_synth_type fmn_hw_synth_type_cheapsynth;
extern const struct fmn_hw_synth_type fmn_hw_synth_type_nicesynth;
 
static const struct fmn_hw_video_type *fmn_hw_video_typev[]={
#if FMN_USE_glx
  &fmn_hw_video_type_glx,
#endif
#if FMN_USE_x11
  &fmn_hw_video_type_x11,
#endif
#if FMN_USE_drmgx
  &fmn_hw_video_type_drmgx,
#endif
#if FMN_USE_drmfb
  &fmn_hw_video_type_drmfb,
#endif
#if FMN_USE_mswm
  &fmn_hw_video_type_mswm,
#endif
#if FMN_USE_macwm
  &fmn_hw_video_type_macwm,
#endif
#if FMN_USE_ttyvideo
  &fmn_hw_video_type_ttyvideo,
#endif
  &fmn_hw_video_type_nullvideo,
};

static const struct fmn_hw_audio_type *fmn_hw_audio_typev[]={
#if FMN_USE_alsa
  &fmn_hw_audio_type_alsa,
#endif
#if FMN_USE_pulse
  &fmn_hw_audio_type_pulse,
#endif
#if FMN_USE_msaudio
  &fmn_hw_audio_type_msaudio,
#endif
#if FMN_USE_macaudio
  &fmn_hw_audio_type_macaudio,
#endif
  &fmn_hw_audio_type_nullaudio,
};

static const struct fmn_hw_input_type *fmn_hw_input_typev[]={
#if FMN_USE_evdev
  &fmn_hw_input_type_evdev,
#endif
#if FMN_USE_machid
  &fmn_hw_input_type_machid,
#endif
#if FMN_USE_mshid
  &fmn_hw_input_type_mshid,
#endif
};

static const struct fmn_hw_synth_type *fmn_hw_synth_typev[]={
#if FMN_USE_nicesynth
  &fmn_hw_synth_type_nicesynth,
#endif
#if FMN_USE_cheapsynth
  &fmn_hw_synth_type_cheapsynth,
#endif
  &fmn_hw_synth_type_nullsynth,
};

/* Access to type registries.
 */
 
#define ACCESSORS(tag) \
  const struct fmn_hw_##tag##_type *fmn_hw_##tag##_type_by_name(const char *name,int namec) { \
    if (!name) return 0; \
    if (namec<0) { namec=0; while (name[namec]) namec++; } \
    const struct fmn_hw_##tag##_type **p=fmn_hw_##tag##_typev; \
    int i=sizeof(fmn_hw_##tag##_typev)/sizeof(void*); \
    for (;i-->0;p++) { \
      if (memcmp(name,(*p)->name,namec)) continue; \
      if ((*p)->name[namec]) continue; \
      return *p; \
    } \
    return 0; \
  } \
  const struct fmn_hw_##tag##_type *fmn_hw_##tag##_type_by_index(int p) { \
    if (p<0) return 0; \
    int c=sizeof(fmn_hw_##tag##_typev)/sizeof(void*); \
    if (p>=c) return 0; \
    return fmn_hw_##tag##_typev[p]; \
  }
ACCESSORS(video)
ACCESSORS(audio)
ACCESSORS(input)
ACCESSORS(synth)
#undef ACCESSORS

/* Device ID allocator.
 * We can afford to do this naively. Let it fail after 2**31 IDs.
 */
 
static int fmn_hw_devid_next_p=1;

int fmn_hw_devid_next() {
  if (fmn_hw_devid_next_p>=INT_MAX) return -1;
  return fmn_hw_devid_next_p++;
}
