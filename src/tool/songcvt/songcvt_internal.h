#ifndef SONGCVT_INTERNAL_H
#define SONGCVT_INTERNAL_H

#include "tool/common/tool_cmdline.h"
#include "opt/fs/fmn_fs.h"
#include "opt/serial/fmn_serial.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#define SONGCVT_CHANNEL_COUNT 16

#define SONGCVT_CHANNEL_FLAG_MODE 0x01 /* +waveid, if 2 */
#define SONGCVT_CHANNEL_FLAG_ENVLO 0x02
#define SONGCVT_CHANNEL_FLAG_ENVHI 0x04

struct songcvt_midi_file {

// Constant after acquiring the file.
  char *path;
  uint8_t *src;
  int srcc;
  
// Mostly constant after dechunking; constant after events read.
  uint16_t format;
  uint16_t track_count;
  uint16_t division;
  struct songcvt_midi_track {
    const uint8_t *v; // points into (src)
    int c;
    int p;
    int term;
    int delay; // <0 if not read yet
    uint8_t status;
  } *trackv;
  int trackc,tracka;
  
  struct songcvt_midi_event {
    int time; // absolute time in input ticks
    int srcp; // absolute position in file (not in chunk)
    uint8_t trackid;
    uint8_t chid;
    uint8_t opcode; // (chid) zero always. (opcode) zero to ignore
    uint8_t a,b;
    struct songcvt_midi_event *partner; // populated after all events read
    const uint8_t *v; // sysex and meta only; points into (src)
    int c;
  } *eventv;
  int eventc,eventa;
  
  int time; // total duration in input ticks (also the 'time' of the last event)
  int tempo; // input tempo us/qnote, defaulted during decode if absent.
  int rate; // output ms/tick
};

extern struct songcvt {
  struct tool_cmdline cmdline;
  struct songcvt_midi_file *midi;
  struct fmn_encoder bin;
  struct fmn_encoder final;
  int outtime; // Running clock during reencode, in output ticks.
  
  struct songcvt_channel {
    uint8_t flags;
    uint8_t mode;
    uint8_t waveid;
    struct songcvt_env {
      uint16_t atktime,dectime,rlstime;
      uint16_t atklevel,suslevel;
    } lo,hi;
  } channelv[SONGCVT_CHANNEL_COUNT];
} songcvt;

int songcvt_reencode(); // populates (bin) from (midi)
int songcvt_generate_output(); // populates (final) from (bin)

/* Most work happens in the context of songcvt_midi_file:
 */

void songcvt_midi_file_del(struct songcvt_midi_file *file);

// Decodes fully and logs all errors.
struct songcvt_midi_file *songcvt_midi_file_new(const char *path);

// These all happen during songcvt_midi_file_new:
int songcvt_midi_file_dechunk(struct songcvt_midi_file *file);
int songcvt_midi_file_read_events(struct songcvt_midi_file *file);
int songcvt_midi_file_associate_events(struct songcvt_midi_file *file);

// Apply adjustments, after decoding a file.
int songcvt_midi_file_adjust(struct songcvt_midi_file *file,const char *path,int require);

#endif
