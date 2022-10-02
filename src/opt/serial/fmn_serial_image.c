/* fmn_serial_image.c
 * These aren't part of the regular fmn_image API because that needs to be super lean for embedded targets.
 * This extension is intended for build-time tooling, dealing with portable images eg PNG.
 */

#include "fmn_serial.h"
#include "game/image/fmn_image.h"
#include <stdlib.h>

/* Get pixel.
 */
 
struct fmn_pixel fmn_image_get_pixel(const struct fmn_image *image,int x,int y) {
  struct fmn_pixel pixel={0};
  if (!image) return pixel;
  if ((x<0)||(x>=image->w)) return pixel;
  if ((y<0)||(y>=image->h)) return pixel;
  
  const uint8_t *row=((uint8_t*)image->v)+image->stride*y;
  switch (image->fmt) {
      
    case FMN_IMAGE_FMT_Y1: {
        row+=x>>3;
        pixel.r=pixel.g=pixel.b=((*row)&(0x80>>(x&7)))?0xff:0x00;
        pixel.a=0xff;
        return pixel;
      }
    case FMN_IMAGE_FMT_Y2: {
        row+=x>>2;
        pixel.r=((*row)>>(6-((x&3)<<1)))&3;
        if ((image->flags&FMN_IMAGE_FLAG_TRANSPARENT)&&(pixel.r==2)) return pixel;
        pixel.r|=pixel.r<<2;
        pixel.r|=pixel.r<<4;
        pixel.g=pixel.b=pixel.r;
        pixel.a=0xff;
        return pixel;
      }
    case FMN_IMAGE_FMT_Y4: {
        row+=x>>1;
        if (x&1) {
          pixel.r=(*row)&0x0f;
          pixel.r|=pixel.r<<4;
        } else {
          pixel.r=(*row)&0xf0;
          pixel.r|=pixel.r>>4;
        }
        pixel.g=pixel.b=pixel.r;
        pixel.a=0xff;
        return pixel;
      }
    case FMN_IMAGE_FMT_Y8: {
        row+=x;
        if ((image->flags&FMN_IMAGE_FLAG_TRANSPARENT)&&(*row==0x81)) return pixel;
        return (struct fmn_pixel){row[0],row[0],row[0],0xff};
      }
    case FMN_IMAGE_FMT_Y16: {
        row+=x<<1;
        return (struct fmn_pixel){row[0],row[0],row[0],0xff};
      }
      
    case FMN_IMAGE_FMT_YA8: {
        row+=x<<1;
        return (struct fmn_pixel){row[0],row[0],row[0],row[1]};
      }
    case FMN_IMAGE_FMT_YA16: {
        row+=x<<2;
        return (struct fmn_pixel){row[0],row[0],row[0],row[2]};
      }
      
    case FMN_IMAGE_FMT_RGB8: {
        row+=x*3;
        return (struct fmn_pixel){row[0],row[1],row[2],0xff};
      }
    case FMN_IMAGE_FMT_RGB16: {
        row+=x*6;
        return (struct fmn_pixel){row[0],row[2],row[4],0xff};
      }

    case FMN_IMAGE_FMT_RGBA: {
        row+=x<<2;
        return (struct fmn_pixel){row[0],row[1],row[2],row[3]};
      }
    case FMN_IMAGE_FMT_RGBA16: {
        row+=x<<3;
        return (struct fmn_pixel){row[0],row[2],row[4],row[6]};
      }
  }
  return pixel;
}

/* Set pixel.
 */
 
void fmn_image_set_pixel(struct fmn_image *image,int x,int y,struct fmn_pixel pixel) {
  if (!image) return;
  if ((x<0)||(x>=image->w)) return;
  if ((y<0)||(y>=image->h)) return;
  
  uint8_t *row=((uint8_t*)image->v)+image->stride*y;
  switch (image->fmt) {
      
    case FMN_IMAGE_FMT_Y1: {
        row+=x>>3;
        if (pixel.r+pixel.g+pixel.b>=384) (*row)|=0x80>>(x&7);
        else (*row)&=~(0x80>>(x&7));
      } return;
    case FMN_IMAGE_FMT_Y2: {
        row+=x>>2;
        uint8_t y=(pixel.r+pixel.g+pixel.b)/3;
        y>>=6;
        int shift=6-((x&3)<<1);
        if (image->flags&FMN_IMAGE_FLAG_TRANSPARENT) {
          if (pixel.a) {
            if (y==2) y=1;
          } else y=2;
        }
        (*row)=((*row)&~(3<<shift))|(y<<shift);
      } return;
    case FMN_IMAGE_FMT_Y4: {
        row+=x>>1;
        uint8_t y=(pixel.r+pixel.g+pixel.b)/3;
        y>>=4;
        if (x&1) (*row)=((*row)&0xf0)|y;
        else (*row)=((*row)&0x0f)|(y<<4);
      } return;
    case FMN_IMAGE_FMT_Y8: {
        row+=x;
        uint8_t y=(pixel.r+pixel.g+pixel.b)/3;
        if (image->flags&FMN_IMAGE_FLAG_TRANSPARENT) {
          if (pixel.a) {
            if (y==0x81) y=0x80;
          } else y=0x81;
        }
        *row=y;
      } return;
    case FMN_IMAGE_FMT_Y16: {
        row+=x<<1;
        row[0]=row[1]=(pixel.r+pixel.g+pixel.b)/3;
      } return;
      
    case FMN_IMAGE_FMT_YA8: {
        row+=x<<1;
        row[0]=(pixel.r+pixel.g+pixel.b)/3;
        row[1]=pixel.a;
      } return;
    case FMN_IMAGE_FMT_YA16: {
        row+=x<<2;
        row[0]=row[1]=(pixel.r+pixel.g+pixel.b)/3;
        row[2]=row[3]=pixel.a;
      } return;
      
    case FMN_IMAGE_FMT_RGB8: {
        row+=x*3;
        row[0]=pixel.r;
        row[1]=pixel.g;
        row[2]=pixel.b;
      } return;
    case FMN_IMAGE_FMT_RGB16: {
        row+=x*6;
        row[0]=row[1]=pixel.r;
        row[2]=row[3]=pixel.g;
        row[4]=row[5]=pixel.b;
      } return;

    case FMN_IMAGE_FMT_RGBA: {
        row+=x<<2;
        if (image->flags&FMN_IMAGE_FLAG_TRANSPARENT) {
          if (pixel.a) ;
          else pixel.r=pixel.g=pixel.b=0;
        }
        row[0]=pixel.r;
        row[1]=pixel.g;
        row[2]=pixel.b;
        row[3]=pixel.a;
      } return;
    case FMN_IMAGE_FMT_RGBA16: {
        row+=x<<3;
        row[0]=row[1]=pixel.r;
        row[2]=row[3]=pixel.g;
        row[4]=row[5]=pixel.b;
        row[6]=row[7]=pixel.a;
      } return;
  }
}

/* Initialize fresh image.
 */
 
int fmn_image_init(struct fmn_image *image,uint8_t fmt,int w,int h) {
  if (!image||image->v) return -1;
  if ((w<1)||(w>FMN_IMAGE_SIZE_LIMIT)) return -1;
  if ((h<1)||(h>FMN_IMAGE_SIZE_LIMIT)) return -1;
  
  image->fmt=fmt;
  image->w=w;
  image->h=h;
  image->flags=FMN_IMAGE_FLAG_OWNV|FMN_IMAGE_FLAG_WRITEABLE;

  switch (fmt) {
    case FMN_IMAGE_FMT_RGBA: image->stride=w<<2; break;
    case FMN_IMAGE_FMT_Y8: image->stride=w; break;
    case FMN_IMAGE_FMT_Y2: image->stride=(w+3)>>2; break;
    case FMN_IMAGE_FMT_Y1: image->stride=(w+7)>>3; break;
    case FMN_IMAGE_FMT_Y16: image->stride=w<<1; break;
    case FMN_IMAGE_FMT_YA8: image->stride=w<<1; break;
    case FMN_IMAGE_FMT_YA16: image->stride=w<<2; break;
    case FMN_IMAGE_FMT_RGB8: image->stride=w*3; break;
    case FMN_IMAGE_FMT_RGB16: image->stride=w*6; break;
    case FMN_IMAGE_FMT_RGBA16: image->stride=w<<3; break;
    default: return -1;
  }
  if (image->stride<1) return -1;
  
  if (!(image->v=calloc(image->stride,h))) return -1;
  
  return 0;
}
