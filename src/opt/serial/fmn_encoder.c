#include "fmn_serial.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

/* Cleanup.
 */
 
void fmn_encoder_cleanup(struct fmn_encoder *encoder) {
  if (!encoder) return;
  if (encoder->v) free(encoder->v);
}

/* Grow buffer.
 */

int fmn_encoder_require(struct fmn_encoder *encoder,int addc) {
  if (addc<1) return 0;
  if (encoder->c>INT_MAX-addc) return -1;
  int na=encoder->c+addc;
  if (na<=encoder->a) return 0;
  if (na<INT_MAX-1024) na=(na+1024)&~1023;
  void *nv=realloc(encoder->v,na);
  if (!nv) return -1;
  encoder->v=nv;
  encoder->a=na;
  return 0;
}

/* Replace content.
 */
 
int fmn_encoder_replace(struct fmn_encoder *encoder,int p,int c,const void *src,int srcc) {
  if (p<0) return p=encoder->c;
  if (c<0) c=encoder->c-p;
  if (p>encoder->c-c) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (((char*)src)[srcc]) srcc++; }
  if (c!=srcc) {
    if (fmn_encoder_require(encoder,srcc-c)<0) return -1;
    memmove(encoder->v+p+srcc,encoder->v+p+c,encoder->c-p-c);
    encoder->c+=srcc-c;
  }
  memcpy(encoder->v+p,src,srcc);
  return 0;
}

/* Append raw data.
 */

int fmn_encode_raw(struct fmn_encoder *encoder,const void *src,int srcc) {
  return fmn_encoder_replace(encoder,encoder->c,0,src,srcc);
}

/* Append formatted text.
 */
 
int fmn_encode_fmt(struct fmn_encoder *encoder,const char *fmt,...) {
  if (!fmt||!fmt[0]) return 0;
  while (1) {
    va_list vargs;
    va_start(vargs,fmt);
    int err=vsnprintf(encoder->v+encoder->c,encoder->a-encoder->c,fmt,vargs);
    if ((err<0)||(err>=INT_MAX)) return -1;
    if (encoder->c<encoder->a-err) {
      encoder->c+=err;
      return err;
    }
    if (fmn_encoder_require(encoder,err+1)<0) return -1;
  }
}

/* Scalars.
 */
 
int fmn_encode_u8(struct fmn_encoder *encoder,uint8_t src) {
  uint8_t tmp[]={src};
  return fmn_encode_raw(encoder,tmp,sizeof(tmp));
}
