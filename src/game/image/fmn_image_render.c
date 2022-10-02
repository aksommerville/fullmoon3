#include "fmn_image.h"
#include <string.h>
#include <stdio.h>

/* Fill rect.
 */
 
void fmn_image_fill_rect(struct fmn_image *image,int16_t x,int16_t y,int16_t w,int16_t h,uint32_t pixel) {
  if (!image) return;
  if (!(image->flags&FMN_IMAGE_FLAG_WRITEABLE)) return;
  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x>image->w-w) w=image->w-x;
  if (y>image->h-h) h=image->h-y;
  if ((w<1)||(h<1)) return;
  switch (image->fmt) {
  
    #if FMN_IMAGE_FMT_SUPPORTED(RGBA)
    case FMN_IMAGE_FMT_RGBA: {
        int16_t wstride=image->stride>>2;
        uint32_t *dstrow=((uint32_t*)image->v)+y*wstride+x;
        for (;h-->0;dstrow+=wstride) {
          uint32_t *dstp=dstrow;
          int16_t xi=w;
          for (;xi-->0;dstp++) *dstp=pixel;
        }
      } break;
    #endif
  
    #if FMN_IMAGE_FMT_SUPPORTED(Y8)
    case FMN_IMAGE_FMT_Y8: {
        uint8_t *dstrow=((uint8_t*)image->v)+y*image->stride+x;
        for (;h-->0;dstrow+=image->stride) memset(dstrow,pixel,w);
      } break;
    #endif
  
    #if FMN_IMAGE_FMT_SUPPORTED(Y2)
    case FMN_IMAGE_FMT_Y2: {
        pixel&=3;
        // If a rowwise memset is possible, do it and then run the generic case for just the horizontal edges.
        int16_t alignl=(x+3)&~3;
        int16_t alignr=(x+w)&~3;
        if (alignr>alignl) {
          uint8_t byte=pixel|pixel<<2; byte|=byte<<4;
          int16_t bc=w>>2;
          uint8_t *dstrow=((uint8_t*)image->v)+y*image->stride+(alignl>>2);
          int8_t yi=h;
          for (;yi-->0;dstrow+=image->stride) memset(dstrow,byte,bc);
          int16_t extral=alignl-x;
          int16_t extrar=x+w-alignr;
          if (extral&&extrar) {
            fmn_image_fill_rect(image,alignr,y,extrar,h,pixel);
            w=extral;
          } else if (extral) {
            w=extral;
          } else if (extrar) {
            x=alignr;
            w=extrar;
          } else {
            return;
          }
        }
        // The horrible generic case:
        uint8_t *dstrow=((uint8_t*)image->v)+y*image->stride+(x>>2);
        uint8_t shift0=6-((x&3)<<1);
        for (;h-->0;dstrow+=image->stride) {
          uint8_t *dstp=dstrow;
          uint8_t shift=shift0;
          int16_t xi=w;
          for (;xi-->0;) {
            (*dstp)=((*dstp)&~(3<<shift))|(pixel<<shift);
            if (shift) shift-=2;
            else { shift=6; dstp++; }
          }
        }
      } break;
    #endif
  }
}

/* Blit.
 */
 
void fmn_image_blit(
  struct fmn_image *dst,int16_t dstx,int16_t dsty,
  const struct fmn_image *src,int16_t srcx,int16_t srcy,
  int16_t w,int16_t h,
  uint8_t xform
) {
  if (!dst||!(dst->flags&FMN_IMAGE_FLAG_WRITEABLE)) return;
  if (!src) return;
  if (dst->fmt!=src->fmt) return;
  
  if (xform&FMN_XFORM_XREV) {
    if (dstx<0) { w+=dstx; dstx=0; }
    if (srcx<0) { w+=srcx; srcx=0; }
    if (dstx>dst->w-w) { srcx+=dstx+w-dst->w; w=dst->w-dstx; }
    if (srcx>src->w-w) { dstx+=srcx+w-src->w; w=src->w-srcx; }
  } else {
    if (dstx<0) { srcx-=dstx; w+=dstx; dstx=0; }
    if (srcx<0) { dstx-=srcx; w+=srcx; srcx=0; }
    if (dstx>dst->w-w) w=dst->w-dstx;
    if (srcx>src->w-w) w=src->w-srcx;
  }
  if (xform&FMN_XFORM_YREV) {
    if (dsty<0) { h+=dsty; dsty=0; }
    if (srcy<0) { h+=srcy; srcy=0; }
    if (dsty>dst->h-h) { srcy+=dsty+h-dst->h; h=dst->h-dsty; }
    if (srcy>src->h-h) { dsty+=srcy+h-src->h; h=src->h-srcy; }
  } else {
    if (dsty<0) { srcy-=dsty; h+=dsty; dsty=0; }
    if (srcy<0) { dsty-=srcy; h+=srcy; srcy=0; }
    if (dsty>dst->h-h) h=dst->h-dsty;
    if (srcy>src->h-h) h=src->h-srcy;
  }
  if ((w<1)||(h<1)) return;

  switch (dst->fmt) {
  
    #if FMN_IMAGE_FMT_SUPPORTED(RGBA)
    case FMN_IMAGE_FMT_RGBA: {
        int16_t dstwstride=dst->stride>>2;
        int16_t srcwstride=src->stride>>2;
        int16_t dstxstride=1;
        if (xform&FMN_XFORM_XREV) {
          dstx+=w-1;
          dstxstride=-1;
        }
        if (xform&FMN_XFORM_YREV) {
          dsty+=h-1;
        }
        uint32_t *dstrow=((uint32_t*)dst->v)+dsty*dstwstride+dstx;
        const uint32_t *srcrow=((uint32_t*)src->v)+srcy*srcwstride+srcx;
        if (xform&FMN_XFORM_YREV) {
          dstwstride=-dstwstride;
        }
        if (src->flags&FMN_IMAGE_FLAG_TRANSPARENT) {
          for (;h-->0;dstrow+=dstwstride,srcrow+=srcwstride) {
            uint32_t *dstp=dstrow;
            const uint32_t *srcp=srcrow;
            int16_t xi=w;
            for (;xi-->0;dstp+=dstxstride,srcp++) {
              if (!*srcp) continue;
              *dstp=*srcp;
            }
          }
        } else {
          for (;h-->0;dstrow+=dstwstride,srcrow+=srcwstride) {
            uint32_t *dstp=dstrow;
            const uint32_t *srcp=srcrow;
            int16_t xi=w;
            for (;xi-->0;dstp+=dstxstride,srcp++) {
              *dstp=*srcp;
            }
          }
        }
      } break;
    #endif
    
    #if FMN_IMAGE_FMT_SUPPORTED(Y8)
    case FMN_IMAGE_FMT_Y8: {
        int16_t dstystride=dst->stride;
        int16_t dstxstride=1;
        if (xform&FMN_XFORM_XREV) {
          dstx+=w-1;
          dstxstride=-1;
        }
        if (xform&FMN_XFORM_YREV) {
          dsty+=h-1;
          dstystride=-dstystride;
        }
        uint8_t *dstrow=((uint8_t*)dst->v)+dsty*dst->stride+dstx;
        const uint8_t *srcrow=((uint8_t*)src->v)+srcy*src->stride+srcx;
        if (src->flags&FMN_IMAGE_FLAG_TRANSPARENT) {
          for (;h-->0;dstrow+=dstystride,srcrow+=src->stride) {
            uint8_t *dstp=dstrow;
            const uint8_t *srcp=srcrow;
            int16_t xi=w;
            for (;xi-->0;dstp+=dstxstride,srcp++) {
              if (*srcp==0x81) continue;
              *dstp=*srcp;
            }
          }
        } else {
          for (;h-->0;dstrow+=dstystride,srcrow+=src->stride) {
            uint8_t *dstp=dstrow;
            const uint8_t *srcp=srcrow;
            int16_t xi=w;
            for (;xi-->0;dstp+=dstxstride,srcp++) {
              *dstp=*srcp;
            }
          }
        }
      } break;
    #endif
    
    #if FMN_IMAGE_FMT_SUPPORTED(Y2)
    case FMN_IMAGE_FMT_Y2: {
    
        // It's likelier than you might think, for horzes and width to end up byte-aligned,
        // and we can optimize that case pretty good.
        if (!(w&3)&&!(srcx&3)&&!(dstx&3)&&!(xform&FMN_XFORM_XREV)) {
          const uint8_t *srcrow=((uint8_t*)src->v)+srcy*src->stride+(srcx>>2);
          int16_t dstystride=dst->stride;
          if (xform&FMN_XFORM_YREV) {
            dsty+=h-1;
            dstystride=-dstystride;
          }
          uint8_t *dstrow=((uint8_t*)dst->v)+dsty*dst->stride+(dstx>>2);
          uint8_t bc=w>>2;
          if (src->flags&FMN_IMAGE_FLAG_TRANSPARENT) {
            for (;h-->0;dstrow+=dstystride,srcrow+=src->stride) {
              uint8_t *dstp=dstrow;
              const uint8_t *srcp=srcrow;
              int16_t xi=bc;
              for (;xi-->0;dstp++,srcp++) {
                uint8_t bits=*srcp;
                uint8_t mask=0;
                if ((bits&0xc0)!=0x80) mask|=0xc0;
                if ((bits&0x30)!=0x20) mask|=0x30;
                if ((bits&0x0c)!=0x08) mask|=0x0c;
                if ((bits&0x03)!=0x02) mask|=0x03;
                if (mask) *dstp=((*dstp)&~mask)|(bits&mask);
              }
            }
          } else {
            for (;h-->0;dstrow+=dstystride,srcrow+=src->stride) {
              memcpy(dstrow,srcrow,bc);
            }
          }
          return;
        }
        
        // The general case is not horrible either.
        // Well. I guess kind of horrible.
        const uint8_t *srcrow=((uint8_t*)src->v)+srcy*src->stride+(srcx>>2);
        int16_t srcshift0=6-((srcx&3)<<1);
        int16_t dstxstride=-2; // bits
        int16_t dstystride=dst->stride;
        if (xform&FMN_XFORM_XREV) {
          dstx+=w-1;
          dstxstride=2;
        }
        if (xform&FMN_XFORM_YREV) {
          dsty+=h-1;
          dstystride=-dstystride;
        }
        uint8_t *dstrow=((uint8_t*)dst->v)+dsty*dst->stride+(dstx>>2);
        int16_t dstshift0=6-((dstx&3)<<1);
        if (src->flags&FMN_IMAGE_FLAG_TRANSPARENT) {
          for (;h-->0;dstrow+=dstystride,srcrow+=src->stride) {
            uint8_t *dstp=dstrow;
            const uint8_t *srcp=srcrow;
            int16_t dstshift=dstshift0;
            int16_t srcshift=srcshift0;
            int16_t xi=w;
            for (;xi-->0;dstshift+=dstxstride,srcshift-=2) {
              if (srcshift<0) { srcshift+=8; srcp++; }
              if (dstshift<0) { dstshift+=8; dstp++; }
              else if (dstshift>=8) { dstshift-=8; dstp--; }
              uint8_t pixel=((*srcp)>>srcshift)&3;
              if (pixel==2) continue;
              (*dstp)=((*dstp)&~(3<<dstshift))|(pixel<<dstshift);
            }
          }
        } else {
          for (;h-->0;dstrow+=dstystride,srcrow+=src->stride) {
            uint8_t *dstp=dstrow;
            const uint8_t *srcp=srcrow;
            int16_t dstshift=dstshift0;
            int16_t srcshift=srcshift0;
            int16_t xi=w;
            for (;xi-->0;dstshift+=dstxstride,srcshift-=2) {
              if (srcshift<0) { srcshift+=8; srcp++; }
              if (dstshift<0) { dstshift+=8; dstp++; }
              else if (dstshift>=8) { dstshift-=8; dstp--; }
              uint8_t pixel=((*srcp)>>srcshift)&3;
              (*dstp)=((*dstp)&~(3<<dstshift))|(pixel<<dstshift);
            }
          }
        }
      } break;
    #endif
    
  }
}

/* Blit tile (convenience).
 */
 
void fmn_image_blit_tile(
  struct fmn_image *dst,int16_t dstx,int16_t dsty,
  const struct fmn_image *src,
  uint8_t tileid,
  uint8_t xform
) {
  if (!src) return;
  int16_t tilesize=src->w>>4;
  fmn_image_blit(
    dst,dstx-(tilesize>>1),dsty-(tilesize>>1),
    src,(tileid&15)*tilesize,(tileid>>4)*tilesize,
    tilesize,tilesize,
    xform
  );
}
