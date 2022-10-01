/* tiny_sd.h
 * Access to filesystem on Tiny SD card.
 */

#ifndef TINY_SD_H
#define TINY_SD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* There is one global file, only one can be in use at a time.
 * In Full Moon, that is the resource archive.
 * Other code can use files, but must close when done.
 * tiny_file_open() implicitly closes whatever's open first.
 */
int8_t tiny_file_open(const char *path);
int8_t tiny_file_open_writeable(const char *path);
int8_t tiny_file_open_if_closed(const char *path);
void tiny_file_close();

int32_t tiny_file_read(void *dst,int32_t dsta,int32_t seek);
int32_t tiny_file_write(const void *src,int32_t srcc,int32_t seek);

void tiny_file_remove(const char *path);

#ifdef __cplusplus
}
#endif

#endif
