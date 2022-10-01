/* fmn_nullsynth.c
 * This could be a lot more concise, but I'm keeping it like this as a reference for new synths.
 */

#include "fmn_hw.h"
#include <string.h>

/* Object definition.
 */
 
struct fmn_hw_synth_nullsynth {
  struct fmn_hw_synth hdr;
};

#define SYNTH ((struct fmn_hw_synth_nullsynth*)synth)

/* Cleanup.
 */

static void _nullsynth_del(struct fmn_hw_synth *synth) {
}

/* Init.
 */
 
static int _nullsynth_init(struct fmn_hw_synth *synth,const struct fmn_hw_synth_params *params) {
  return 0;
}

/* Generate signal.
 */
 
static void _nullsynth_update(int16_t *v,int c,struct fmn_hw_synth *synth) {
  memset(v,0,c<<1);
}

/* Configure.
 */
 
static int _nullsynth_configure(struct fmn_hw_synth *synth,const void *v,int c) {
  return 0;
}

/* Receive MIDI events.
 */
 
static int _nullsynth_serial_midi(struct fmn_hw_synth *synth,const void *v,int c) {
  return 0;
}

/* Begin song.
 */
 
static int _nullsynth_play_song(struct fmn_hw_synth *synth,const void *v,int c) {
  return 0;
}

/* Play one-off note.
 */
 
static void _nullsynth_note(struct fmn_hw_synth *synth,uint8_t programid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms) {
}

/* Stop all voices cold.
 */
 
static void _nullsynth_silence(struct fmn_hw_synth *synth) {
}

/* Release all held notes.
 */
 
static void _nullsynth_release_all(struct fmn_hw_synth *synth) {
}

/* Type definition.
 */
 
const struct fmn_hw_synth_type fmn_hw_synth_type_nullsynth={
  .name="nullsynth",
  .desc="Dummy synthesizer that only produces silence.",
  .by_request_only=1,
  .objlen=sizeof(struct fmn_hw_synth_nullsynth),
  .del=_nullsynth_del,
  .init=_nullsynth_init,
  .update=_nullsynth_update,
  .configure=_nullsynth_configure,
  .serial_midi=_nullsynth_serial_midi,
  .play_song=_nullsynth_play_song,
  .note=_nullsynth_note,
  .silence=_nullsynth_silence,
  .release_all=_nullsynth_release_all,
};
