/* fmn_cheapsynth.h
 * A simple synthesizer that can run on the Tiny.
 */
 
#ifndef FMN_CHEAPSYNTH_H
#define FMN_CHEAPSYNTH_H

#include <stdint.h>

void fmn_cheapsynth_init(uint16_t rate,uint8_t chanc);
void fmn_cheapsynth_update(int16_t *v,int16_t c);

void fmn_cheapsynth_play_song(const void *v,uint16_t c);
void fmn_cheapsynth_pause_song(uint8_t pause);
void fmn_cheapsynth_note(uint8_t chid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms);
void fmn_cheapsynth_note_on(uint8_t chid,uint8_t noteid,uint8_t velocity);
void fmn_cheapsynth_note_off(uint8_t chid,uint8_t noteid);
void fmn_cheapsynth_config(uint8_t chid,uint8_t k,uint8_t v);
void fmn_cheapsynth_silence();
void fmn_cheapsynth_release_all();

#endif
