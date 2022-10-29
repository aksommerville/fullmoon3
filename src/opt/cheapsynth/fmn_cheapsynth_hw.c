/* fmn_cheapsynth_hw.c
 * Interface to our 'hw' unit.
 * Cheapsynth itself can run fine without this file.
 */
 
#include "fmn_cheapsynth_internal.h"
#include "opt/hw/fmn_hw.h"
#include <string.h>
#include <stdio.h>

/* Object definition.
 */
 
struct fmn_hw_synth_cheapsynth {
  struct fmn_hw_synth hdr;
};

#define SYNTH ((struct fmn_hw_synth_cheapsynth*)synth)

/* Init.
 */
 
static int _cheapsynth_init(struct fmn_hw_synth *synth,const struct fmn_hw_synth_params *params) {
  
  if (params) {
    synth->rate=params->rate;
    synth->chanc=params->chanc;
  }
  if ((synth->rate<200)||(synth->rate>0xffff)) synth->rate=22050;
  if ((synth->chanc<1)||(synth->chanc>2)) synth->chanc=1;
  
  fmn_cheapsynth_init(synth->rate,synth->chanc);
  
  return 0;
}

/* Configure.
 */
 
static int _cheapsynth_configure(struct fmn_hw_synth *synth,const void *v,int c) {
  fprintf(stderr,"%s c=%d\n",__func__,c);
  return 0;
}

/* Receive live MIDI.
 */
 
static int _cheapsynth_serial_midi(struct fmn_hw_synth *synth,const void *v,int c) {
  fprintf(stderr,"%s c=%d\n",__func__,c);
  return 0;
}

/* Trivial forwards to synthesizer.
 */

static void _cheapsynth_update(int16_t *v,int c,struct fmn_hw_synth *synth) {
  fmn_cheapsynth_update(v,c);
}
 
static int _cheapsynth_play_song(struct fmn_hw_synth *synth,const void *v,int c) {
  fmn_cheapsynth_play_song(v,c);
  return 0;
}

static int _cheapsynth_pause_song(struct fmn_hw_synth *synth,uint8_t pause) {
  fmn_cheapsynth_pause_song(pause);
  return 0;
}
 
static void _cheapsynth_note(struct fmn_hw_synth *synth,uint8_t programid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms) {
  fmn_cheapsynth_note(programid,noteid,velocity,duration_ms);
}
 
static void _cheapsynth_silence(struct fmn_hw_synth *synth) {
  fmn_cheapsynth_silence();
}
 
static void _cheapsynth_release_all(struct fmn_hw_synth *synth) {
  fmn_cheapsynth_release_all();
}
  
/* Type definition.
 */
 
const struct fmn_hw_synth_type fmn_hw_synth_type_cheapsynth={
  .name="cheapsynth",
  .desc="Simple synthesizer, minimal requirements.",
  .objlen=sizeof(struct fmn_hw_synth_cheapsynth),
  .init=_cheapsynth_init,
  .update=_cheapsynth_update,
  .configure=_cheapsynth_configure,
  .serial_midi=_cheapsynth_serial_midi,
  .play_song=_cheapsynth_play_song,
  .pause_song=_cheapsynth_pause_song,
  .note=_cheapsynth_note,
  .silence=_cheapsynth_silence,
  .release_all=_cheapsynth_release_all,
};
