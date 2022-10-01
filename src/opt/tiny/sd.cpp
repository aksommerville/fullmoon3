#include <stdint.h>
#include <SD.h>
#include "tiny_sd.h"

/* Globals.
 */
 
static uint8_t sdinit=0;
static File tiny_file=File();

/* Initialize if necessary.
 */
 
static int8_t tiny_sd_require() {
  if (!sdinit) {
    if (SD.begin()<0) return -1;
    sdinit=1;
  }
  return 0;
}

/* Open file.
 */
 
int8_t tiny_file_open(const char *path) {
  if (tiny_sd_require()<0) return -1;
  if (tiny_file) {
    tiny_file.close();
  }
  tiny_file=SD.open(path);
  if (!tiny_file) return -1;
  return 0;
}
 
int8_t tiny_file_open_writeable(const char *path) {
  if (tiny_sd_require()<0) return -1;
  if (tiny_file) {
    tiny_file.close();
  }
  tiny_file=SD.open(path,FILE_WRITE);
  if (!tiny_file) return -1;
  return 0;
}

int8_t tiny_file_open_if_closed(const char *path) {
  if (tiny_sd_require()<0) return -1;
  if (tiny_file) return 0;
  tiny_file=SD.open(path);
  if (!tiny_file) return -1;
  return 1;
}

void tiny_file_close() {
  if (tiny_file) {
    tiny_file.close();
  }
}

/* Read file.
 */
 
int32_t tiny_file_read(void *dst,int32_t dsta,int32_t seek) {
  if (dsta>0xffff) dsta=0xffff;
  if (tiny_sd_require()<0) return -1;
  if (!tiny_file) return -1;
  if (seek) {
    if (tiny_file.seek(seek)<0) return -1;
  }
  int32_t dstc=tiny_file.read(dst,dsta);
  return dstc;
}

/* Write file.
 */
 
int32_t tiny_file_write(const void *src,int32_t srcc,int32_t seek) {
  if (srcc<0) return -1;
  if (tiny_sd_require()<0) return -1;
  if (!tiny_file) return -1;
  if (seek) {
    int32_t p=tiny_file.seek(seek);
    if (p<0) return -1;
  }
  size_t err=tiny_file.write((const uint8_t*)src,(size_t)srcc);
  if (err<0) return -1;
  return 0;
}

/* Delete file.
 */
 
void tiny_file_remove(const char *path) {
  if (tiny_sd_require()<0) return;
  SD.remove(path);
}
