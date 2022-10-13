#include "opt/serial/fmn_serial.h"
#include "game/image/fmn_image.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <zlib.h>

#define PNG_SIZE_LIMIT 4096

/* Decoder context.
 */
 
struct png_decoder {
  const char *refname;
  
  // IHDR
  int w,h;
  uint8_t depth,colortype;
  
  // Verbatim chunks.
  void *plte,*trns;
  int pltec,trnsc;
  
  // IDAT
  z_stream z;
  int zinit;
  int stride,xstride;
  uint8_t *rowbuf;
  uint8_t *pixels;
  int y;
};

/* Cleanup.
 */
 
static void png_decoder_cleanup(struct png_decoder *png) {
  if (png->plte) free(png->plte);
  if (png->trns) free(png->trns);
  if (png->rowbuf) free(png->rowbuf);
  if (png->pixels) free(png->pixels);
  if (png->zinit) inflateEnd(&png->z);
}

/* Unfilter.
 */
 
static inline void png_unfilter_NONE(uint8_t *dst,const uint8_t *src,const uint8_t *prv,int stride,int xstride) {
  memcpy(dst,src,stride);
}
 
static inline void png_unfilter_SUB(uint8_t *dst,const uint8_t *src,const uint8_t *prv,int stride,int xstride) {
  int i=0;
  for (;i<xstride;i++) dst[i]=src[i];
  for (;i<stride;i++) dst[i]=src[i]+dst[i-xstride];
}
 
static inline void png_unfilter_UP(uint8_t *dst,const uint8_t *src,const uint8_t *prv,int stride,int xstride) {
  if (!prv) memcpy(dst,src,stride);
  else {
    int i=0;
    for (;i<stride;i++) dst[i]=src[i]+prv[i];
  }
}
 
static inline void png_unfilter_AVG(uint8_t *dst,const uint8_t *src,const uint8_t *prv,int stride,int xstride) {
  int i=0;
  if (!prv) {
    for (;i<xstride;i++) dst[i]=src[i];
    for (;i<stride;i++) dst[i]=src[i]+(dst[i-xstride]>>1);
  } else {
    for (;i<xstride;i++) dst[i]=src[i]+(prv[i]>>1);
    for (;i<stride;i++) dst[i]=src[i]+((prv[i]+dst[i-xstride])>>1);
  }
}

static inline uint8_t png_paeth(uint8_t a,uint8_t b,uint8_t c) {
  int p=a+b-c;
  int pa=p-a; if (pa<0) pa=-pa;
  int pb=p-b; if (pb<0) pb=-pb;
  int pc=p-c; if (pc<0) pc=-pc;
  if ((pa<=pb)&&(pa<=pc)) return a;
  if (pb<=pc) return b;
  return c;
}
 
static inline void png_unfilter_PAETH(uint8_t *dst,const uint8_t *src,const uint8_t *prv,int stride,int xstride) {
  int i=0;
  if (!prv) {
    for (;i<xstride;i++) dst[i]=src[i];
    for (;i<stride;i++) dst[i]=src[i]+dst[i-xstride];
  } else {
    for (;i<xstride;i++) dst[i]=src[i]+prv[i];
    for (;i<stride;i++) dst[i]=src[i]+png_paeth(dst[i-xstride],prv[i],prv[i-xstride]);
  }
}

/* Add rowbuf to the output and prepare for the next row.
 */
 
static int png_decode_row(struct png_decoder *png) {
  if (png->y<png->h) {
    uint8_t *dst=png->pixels+png->y*png->stride;
    uint8_t *prv=0;
    if (png->y) prv=dst-png->stride;
    const uint8_t *src=png->rowbuf+1;
    switch (png->rowbuf[0]) {
      case 0: png_unfilter_NONE(dst,src,prv,png->stride,png->xstride); break;
      case 1: png_unfilter_SUB(dst,src,prv,png->stride,png->xstride); break;
      case 2: png_unfilter_UP(dst,src,prv,png->stride,png->xstride); break;
      case 3: png_unfilter_AVG(dst,src,prv,png->stride,png->xstride); break;
      case 4: png_unfilter_PAETH(dst,src,prv,png->stride,png->xstride); break;
      default: {
          if (png->refname) fprintf(stderr,"%s: Unexpected filter byte 0x%02x at row %d/%d\n",png->refname,png->rowbuf[0],png->y,png->h);
          return -2;
        }
    }
    png->y++;
  }
  png->z.next_out=(Bytef*)png->rowbuf;
  png->z.avail_out=1+png->stride;
  return 0;
}

/* IDAT.
 */
 
static int png_decode_IDAT(struct png_decoder *png,const void *src,int srcc) {
  if (!png->zinit) {
    if (png->refname) fprintf(stderr,"%s: IDAT before IHDR\n",png->refname);
    return -2;
  }
  
  png->z.next_in=(Bytef*)src;
  png->z.avail_in=srcc;
  while (png->z.avail_in) {
    
    if (!png->z.avail_out) {
      int err=png_decode_row(png);
      if (err<0) return err;
    }
    
    int err=inflate(&png->z,Z_NO_FLUSH);
    if (err<0) {
      err=inflate(&png->z,Z_SYNC_FLUSH);
      if (err<0) {
        if (png->refname) fprintf(stderr,"%s: zlib error %d\n",png->refname,err);
        return -2;
      }
    }
  }
  return 0;
}

/* IHDR.
 */
 
static int png_decode_IHDR(struct png_decoder *png,const uint8_t *src,int srcc) {
  if (png->w) {
    if (png->refname) fprintf(stderr,"%s:WARNING: Multiple IHDR. Keeping the first.\n",png->refname);
    return 0;
  }
  if (srcc<13) {
    if (png->refname) fprintf(stderr,"%s: Short IHDR (%d<13)\n",png->refname,srcc);
    return -2;
  }
  
  png->w=(src[0]<<24)|(src[1]<<16)|(src[2]<<8)|src[3];
  png->h=(src[4]<<24)|(src[5]<<16)|(src[6]<<8)|src[7];
  png->depth=src[8];
  png->colortype=src[9];
  uint8_t compression=src[10];
  uint8_t filter=src[11];
  uint8_t interlace=src[12];
  
  if (
    (png->w<1)||(png->w>PNG_SIZE_LIMIT)||
    (png->h<1)||(png->h>PNG_SIZE_LIMIT)
  ) {
    if (png->refname) fprintf(stderr,"%s: Unacceptable dimensions %dx%d\n",png->refname,png->w,png->h);
    return -2;
  }
  
  // This formula permits everything mandated by the spec, and also some other oddballs.
  int chanc=0;
  switch (png->colortype) {
    case 0: chanc=1; break;
    case 2: chanc=3; break;
    case 3: chanc=1; break;
    case 4: chanc=2; break;
    case 6: chanc=4; break;
  }
  int pixelsize=chanc*png->depth;
  switch (pixelsize) {
    case 1: case 2: case 4: case 8: case 16: case 24: case 32: case 48: case 64: break;
    default: {
        if (png->refname) fprintf(stderr,"%s: Unsupported depth/colortype %d/%d\n",png->refname,png->depth,png->colortype);
        return -2;
      }
  }
  
  if (interlace) {
    // Here we violate the spec, interlace==1 (Adam7) we really are supposed to support.
    // I'm not going to support Adam7 due to the extra complexity, and we're not streaming images over a network. Don't interlace.
    if (png->refname) fprintf(stderr,"%s: Interlaced PNG not supported (%d).\n",png->refname,interlace);
    return -2;
  }
  if (compression||filter) {
    // Only zero is defined for both of these.
    if (png->refname) fprintf(stderr,"%s: Unsupported compression/filter %d/%d\n",png->refname,compression,filter);
    return -2;
  }
  
  // Look good? Let's prepare the compressor.
  if (inflateInit(&png->z)<0) {
    if (png->refname) fprintf(stderr,"%s: Failed to initialize zlib\n",png->refname);
    return -2;
  }
  png->zinit=1;
  
  png->xstride=(pixelsize<8)?1:(pixelsize>>3);
  png->stride=(png->w*pixelsize+7)>>3;
  if (!(png->rowbuf=malloc(1+png->stride))) return -1;
  if (!(png->pixels=malloc(png->stride*png->h))) return -1;
  
  png->y=0;
  png->z.next_out=(Bytef*)png->rowbuf;
  png->z.avail_out=1+png->stride;
  
  return 0;
}

/* Receive verbatim chunks.
 */
 
static int png_decode_tRNS(struct png_decoder *png,const void *src,int srcc) {
  if (png->trns) {
    if (png->refname) fprintf(stderr,"%s:WARNING: Multiple tRNS. Keeping the first.\n",png->refname);
    return 0;
  }
  if (!(png->trns=malloc(srcc))) return -1;
  memcpy(png->trns,src,srcc);
  png->trnsc=srcc;
  return 0;
}

static int png_decode_PLTE(struct png_decoder *png,const void *src,int srcc) {
  if (png->plte) {
    if (png->refname) fprintf(stderr,"%s:WARNING: Multiple PLTE. Keeping the first.\n",png->refname);
    return 0;
  }
  if (!(png->plte=malloc(srcc))) return -1;
  memcpy(png->plte,src,srcc);
  png->pltec=srcc;
  return 0;
}

/* Generate image. Decoder must be finished and valid.
 */
 
static int png_generate_image(struct fmn_image *image,struct png_decoder *png) {

  image->fmt=0;
  switch (png->colortype) {
    case 0: case 3: switch (png->depth) {
        case 1: image->fmt=FMN_IMAGE_FMT_Y1; break;
        case 2: image->fmt=FMN_IMAGE_FMT_Y2; break;
        case 4: image->fmt=FMN_IMAGE_FMT_Y4; break;
        case 8: image->fmt=FMN_IMAGE_FMT_Y8; break;
        case 16: image->fmt=FMN_IMAGE_FMT_Y16; break;
      } break;
    case 2: switch (png->depth) {
        case 8: image->fmt=FMN_IMAGE_FMT_RGB8; break;
        case 16: image->fmt=FMN_IMAGE_FMT_RGB16; break;
      } break;
    case 4: switch (png->depth) {
        case 8: image->fmt=FMN_IMAGE_FMT_YA8; break;
        case 16: image->fmt=FMN_IMAGE_FMT_YA16; break;
      } break;
    case 6: switch (png->depth) {
        case 8: image->fmt=FMN_IMAGE_FMT_RGBA; break;
        case 16: image->fmt=FMN_IMAGE_FMT_RGBA16; break;
      } break;
  }
  if (!image->fmt) {
    if (png->refname) fprintf(stderr,"%s: No Full Moon image format for colortype=%d depth=%d\n",png->refname,png->colortype,png->depth);
    return -2;
  }
  
  image->v=png->pixels;
  png->pixels=0;
  image->w=png->w;
  image->h=png->h;
  image->stride=png->stride;
  image->flags=FMN_IMAGE_FLAG_WRITEABLE|FMN_IMAGE_FLAG_OWNV;
  switch (png->colortype) {
    case 4: case 6: image->flags|=FMN_IMAGE_FLAG_TRANSPARENT; break;
  }
  return 0;
}

/* Finish decoding and produce the final image.
 */
 
static int png_decode_finish(struct fmn_image *image,struct png_decoder *png) {
  if (!png->zinit) {
    if (png->refname) fprintf(stderr,"%s: No IHDR\n",png->refname);
    return 0;
  }
  
  // Flush the decompressor.
  png->z.next_in=0;
  png->z.avail_in=0;
  while (1) {
    if (!png->z.avail_out) {
      if (png_decode_row(png)<0) return 0;
    }
    int err=inflate(&png->z,Z_FINISH);
    if (err<0) {
      if (png->refname) fprintf(stderr,"%s: zlib error %d\n",png->refname,err);
      return 0;
    }
    if (err==Z_STREAM_END) {
      if (!png->z.avail_out&&(png_decode_row(png)<0)) return 0;
      break;
    }
  }
  
  if (png->y<png->h) {
    if (png->refname) fprintf(stderr,"%s:WARNING: Image data ended at row %d/%d\n",png->refname,png->y,png->h);
  }
  
  return png_generate_image(image,png);
}

/* Decode PNG, main entry point.
 */
 
int fmn_png_decode(struct fmn_image *image,const void *src,int srcc,const char *refname) {

  if (!src||(srcc<8)||memcmp(src,"\x89PNG\r\n\x1a\n",8)) {
    if (refname) fprintf(stderr,"%s: PNG signature mismatch\n",refname);
    return -2;
  }
  
  struct png_decoder png={
    .refname=refname,
  };
  
  const uint8_t *SRC=src;
  int srcp=8;
  while (srcp<srcc) {
  
    int srcp0=srcp;
    if (srcp>srcc-8) {
      if (refname) fprintf(stderr,"%s: Unexpected end of file\n",refname);
      png_decoder_cleanup(&png);
      return -2;
    }
    int chunklen=(SRC[srcp]<<24)|(SRC[srcp+1]<<16)|(SRC[srcp+2]<<8)|SRC[srcp+3];
    srcp+=4;
    const uint8_t *chunkid=SRC+srcp;
    srcp+=4;
    if ((chunklen<0)||(srcp>srcc-chunklen)) {
      if (refname) fprintf(stderr,"%s: Invalid chunk length %d around %d/%d\n",refname,chunklen,srcp0,srcc);
      png_decoder_cleanup(&png);
      return -2;
    }
    const uint8_t *chunk=SRC+srcp;
    srcp+=chunklen;
    srcp+=4; // skip crc
    
    int err=0;
         if (!memcmp(chunkid,"IHDR",4)) err=png_decode_IHDR(&png,chunk,chunklen);
    else if (!memcmp(chunkid,"IEND",4)) break;
    else if (!memcmp(chunkid,"IDAT",4)) err=png_decode_IDAT(&png,chunk,chunklen);
    else if (!memcmp(chunkid,"tRNS",4)) err=png_decode_tRNS(&png,chunk,chunklen);
    else if (!memcmp(chunkid,"PLTE",4)) err=png_decode_PLTE(&png,chunk,chunklen);
    // Could assert no unknown critial chunks here, but whatever.
    if (err<0) {
      if (refname&&(err!=-2)) fprintf(stderr,"%s: Unspecified error decoding '%.4s' at %d/%d\n",refname,(char*)chunkid,srcp0,srcc);
      png_decoder_cleanup(&png);
      return -2;
    }
  }
  
  int err=png_decode_finish(image,&png);
  png_decoder_cleanup(&png);
  return err;
}
