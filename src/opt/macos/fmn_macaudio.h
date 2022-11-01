/* fmn_macaudio.h
 * Simple API to produce an audio output stream for MacOS.
 * Link: -framework AudioUnit
 */

#ifndef FMN_MACAUDIO_H
#define FMN_MACAUDIO_H

#include <stdint.h>

int fmn_macaudio_init(int rate,int chanc,void (*cb)(int16_t *dst,int dstc));
void fmn_macaudio_quit();

int fmn_macaudio_lock();
int fmn_macaudio_unlock();

#endif
