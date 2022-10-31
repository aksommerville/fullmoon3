#include "fmn_image.h"
#include <stdlib.h>
#include <stdio.h>

#if FMN_USE_tiny
  #define BIG_ENDIAN 4321
  #define LITTLE_ENDIAN 1234
  #define BYTE_ORDER LITTLE_ENDIAN
#elif FMN_USE_macos
  #include <machine/endian.h>
#else
  #include <endian.h>
#endif

/* Lifecycle.
 */
 
void fmn_image_del(struct fmn_image *image) {
  if (!image) return;
  if (!image->refc) return;
  if (image->refc-->1) return;
  if (image->v&&(image->flags&FMN_IMAGE_FLAG_OWNV)) free(image->v);
  free(image);
}

int8_t fmn_image_ref(struct fmn_image *image) {
  if (!image) return -1;
  if (image->refc<1) return -1;
  if (image->refc==0xff) return -1;
  image->refc++;
  return 0;
}

/* New, allocating.
 */
 
struct fmn_image *fmn_image_new_alloc(uint8_t fmt,int16_t w,int16_t h) {
  int16_t stride=fmn_image_minimum_stride(fmt,w,h);
  int16_t len=fmn_image_measure(fmt,w,h);
  if ((stride<1)||(len<1)) return 0;
  
  struct fmn_image *image=calloc(1,sizeof(struct fmn_image));
  if (!image) return 0;
  
  image->refc=1;
  image->flags=FMN_IMAGE_FLAG_WRITEABLE;
  if (!(image->v=calloc(1,len))) {
    fmn_image_del(image);
    return 0;
  }
  image->w=w;
  image->h=h;
  image->stride=stride;
  image->fmt=fmt;
  
  return image;
}

/* Measure.
 */
 
int16_t fmn_image_measure(uint8_t fmt,int16_t w,int16_t h) {
  // Call out exceptions here, anything that isn't raw LRTB with byte-aligned rows.
  return fmn_image_minimum_stride(fmt,w,h)*h;
}

int16_t fmn_image_minimum_stride(uint8_t fmt,int16_t w,int16_t h) {
  if ((w<1)||(w>FMN_IMAGE_SIZE_LIMIT)) return 0;
  if ((h<1)||(h>FMN_IMAGE_SIZE_LIMIT)) return 0;
  switch (fmt) {
    case FMN_IMAGE_FMT_RGBA: return w*4;
    case FMN_IMAGE_FMT_Y8: return w;
    case FMN_IMAGE_FMT_Y2: return (w+3)>>2;
  }
  return 0;
}

/* Pixel format.
 */
 
uint32_t fmn_pixel_from_rgba(uint8_t fmt,uint8_t r,uint8_t g,uint8_t b,uint8_t a) {
  switch (fmt) {
    #if BYTE_ORDER==BIG_ENDIAN
      case FMN_IMAGE_FMT_RGBA: return (r<<24)|(g<<16)|(b<<8)|a;
    #else
      case FMN_IMAGE_FMT_RGBA: return r|(g<<8)|(b<<16)|(a<<24);
    #endif
    case FMN_IMAGE_FMT_Y8: return (r+g+b)/3;
    case FMN_IMAGE_FMT_Y2: if (!a) return 2; r=(r+g+b)/3; r>>=6; if (r==2) return 1; return r;
  }
  return 0;
}

uint8_t fmn_pixel_size_for_image_format(uint8_t fmt) {
  switch (fmt) {
    case FMN_IMAGE_FMT_RGBA: return 32;
    case FMN_IMAGE_FMT_Y8: return 8;
    case FMN_IMAGE_FMT_Y2: return 2;
  }
  return 0;
}
