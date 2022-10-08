#include "songcvt_internal.h"

/* Delete.
 */
 
void songcvt_midi_file_del(struct songcvt_midi_file *file) {
  if (!file) return;
  
  if (file->path) free(file->path);
  if (file->src) free(file->src);
  if (file->trackv) free(file->trackv);
  if (file->eventv) free(file->eventv);
  
  free(file);
}

/* New.
 */

struct songcvt_midi_file *songcvt_midi_file_new(const char *path) {
  if (!path||!path[0]) return 0;
  struct songcvt_midi_file *file=calloc(1,sizeof(struct songcvt_midi_file));
  if (!file) return 0;
  
  if (!(file->path=strdup(path))) {
    songcvt_midi_file_del(file);
    return 0;
  }
  
  if ((file->srcc=fmn_file_read(&file->src,path))<0) {
    fprintf(stderr,"%s: Failed to read file\n",path);
    songcvt_midi_file_del(file);
    return 0;
  }
  
  int err;
  if (
    ((err=songcvt_midi_file_dechunk(file))<0)||
    ((err=songcvt_midi_file_read_events(file))<0)||
    ((err=songcvt_midi_file_associate_events(file))<0)||
  0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error decoding MIDI file\n",path);
    songcvt_midi_file_del(file);
    return 0;
  }
  
  return file;
}
