/* fmn_web_stubs.c
 * I don't want to introduce a bunch of unnecessary cruft.
 * So instead, the few libc things that the app does, just emulate them here.
 */
 
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

FILE *const stderr=0;

void fmn_web_external_console_log(const char *src);

int fprintf(FILE *f,const char *fmt,...) {
  va_list vargs;
  va_start(vargs,fmt);
  char tmp[256];
  int tmpc=0;
  while (*fmt) {
    if (*fmt=='%') {
      fmt+=2;
      int v=va_arg(vargs,int);
      if (v<0) { tmp[tmpc++]='-'; v=-v; }
      if (v>=1000000) tmp[tmpc++]='0'+(v/1000000)%10;
      if (v>= 100000) tmp[tmpc++]='0'+(v/ 100000)%10;
      if (v>=  10000) tmp[tmpc++]='0'+(v/  10000)%10;
      if (v>=   1000) tmp[tmpc++]='0'+(v/   1000)%10;
      if (v>=    100) tmp[tmpc++]='0'+(v/    100)%10;
      if (v>=     10) tmp[tmpc++]='0'+(v/     10)%10;
                      tmp[tmpc++]='0'+(v/      1)%10;
    } else {
      tmp[tmpc++]=*fmt++;
    }
    if (tmpc>=200) { tmpc=0; break; }
  }
  tmp[tmpc]=0;
  fmn_web_external_console_log(tmp);
  return 0;
}

size_t fwrite(const void *src,size_t c,size_t len,FILE *f) {
  return 0;
}

int fputc(int ch,FILE *f) {
  return 0;
}

void *memcpy(void *dst,const void *src,unsigned long c) {
  uint8_t *DST=dst;
  const uint8_t *SRC=src;
  for (;c-->0;DST++,SRC++) *DST=*SRC;
  return dst;
}

void *memset(void *dst,int src,unsigned long c) {
  uint8_t *DST=dst;
  for (;c-->0;DST++) *DST=src;
  return dst;
}

void free(void *v) {
}

void *calloc(size_t c,size_t len) {
  return 0;
}
