#include "fmn_image.h"
#include <string.h>
#include <stdio.h>

/* Pans.
 */
 
static void fmn_image_transition_pan_left(
  struct fmn_image *to,
  const struct fmn_image *from,
  uint8_t p,uint8_t c
) {
  int16_t d=(p*to->w)/c;
  if (d<1) return;
  uint8_t *torow=to->v;
  const uint8_t *torrow=torow+(d>>2);
  int16_t yi=to->h;
  int16_t selfcopy_pixelc=to->w-d;
  int16_t selfcopy_bytec=selfcopy_pixelc>>2;
  uint8_t selfcopy_nextshift=6-((d&3)<<1);
  uint8_t selfcopy_shift=8-selfcopy_nextshift;
  for (;yi-->0;torow+=to->stride,torrow+=to->stride) {
    int16_t xi=selfcopy_bytec;
    uint8_t *top=torow;
    const uint8_t *torp=torrow;
    for (;xi-->0;top++,torp++) {
      *top=(torp[0]<<selfcopy_shift)|(torp[1]>>selfcopy_nextshift);
    }
    if (selfcopy_shift!=8) {
      top[0]=(torp[0]<<selfcopy_shift);
    }
  }
  fmn_image_blit(to,to->w-d-1,0,from,0,0,to->w,to->h,0);
}
 
static void fmn_image_transition_pan_right(
  struct fmn_image *to,
  const struct fmn_image *from,
  uint8_t p,uint8_t c
) {
  int16_t d=(p*to->w)/c;
  if (d<1) return;
  int16_t yi=to->h;
  int16_t selfcopy_pixelc=to->w-d;
  int16_t selfcopy_bytec=(selfcopy_pixelc+3)>>2;
  uint8_t *torow=to->v+(to->w>>2)-1;
  const uint8_t *torrow=to->v+((selfcopy_pixelc+3)>>2)-1;
  uint8_t selfcopy_shift=((d&3)<<1);
  uint8_t selfcopy_nextshift=8-selfcopy_shift;
  for (;yi-->0;torow+=to->stride,torrow+=to->stride) {
    int16_t xi=selfcopy_bytec;
    uint8_t *top=torow;
    const uint8_t *torp=torrow;
    for (;xi-->0;top--,torp--) {
      *top=(torp[0]>>selfcopy_shift)|(torp[-1]<<selfcopy_nextshift);
    }
  }
  fmn_image_blit(to,0,0,from,to->w-d,0,to->w,to->h,0);
}
 
static void fmn_image_transition_pan_up(
  struct fmn_image *to,
  const struct fmn_image *from,
  uint8_t p,uint8_t c
) {
  int16_t d=(p*to->h)/c;
  if (d<1) return;
  uint8_t *dst=to->v;
  const uint8_t *src=dst+d*to->stride;
  int16_t i=to->h-d;
  for (;i-->0;dst+=to->stride,src+=to->stride) {
    memcpy(dst,src,to->stride);
  }
  fmn_image_blit(to,0,to->h-d,from,0,0,to->w,to->h,0);
}
 
static void fmn_image_transition_pan_down(
  struct fmn_image *to,
  const struct fmn_image *from,
  uint8_t p,uint8_t c
) {
  int16_t d=(p*to->h)/c;
  if (d<1) return;
  const uint8_t *src=((uint8_t*)to->v)+(to->h-d-1)*to->stride;
  uint8_t *dst=((uint8_t*)to->v)+(to->h-1)*to->stride;
  int16_t i=to->h-d;
  for (;i-->0;src-=to->stride,dst-=to->stride) {
    memcpy(dst,src,to->stride);
  }
  fmn_image_blit(to,0,0,from,0,to->h-d,to->w,d,0);
}

/* Dissolve with intermediate blackout.
 */
 
static void fmn_image_dissolve_blackout(
  struct fmn_image *image,
  uint8_t p,uint8_t c
) {
  // Edge cases.
  if (!p) return;
  if (p>=c) {
    fmn_image_fill_rect(image,0,0,image->w,image->h,0);
    return;
  }
  uint8_t pixelsize=fmn_pixel_size_for_image_format(image->fmt);
  if (!pixelsize) return;
  
  // Build up a mask of prime length, with a count of ones proportionate to (p/c).
  uint8_t mask[79]={0};
  const uint8_t step=7;
  uint32_t maskp=0,i=(p*sizeof(mask))/c;
  for (;i-->0;maskp+=step) mask[maskp%sizeof(mask)]=1;
  maskp=0;
  
  // We're going to apply the mask blockwise, in 32x32 pixel blocks.
  // It's important that the block size be coprime to the mask length; mask length being prime, that's assured.
  // It must also be square, and land on byte boundaries. (again, use powers of two, and it all works out).
  // We happen to know that we'll only be asked to transition framebuffers whose dimensions are multiples of 32x32.
  // If that should ever change, it's no big deal, but we'd have to range-check all the writes on the last column and row of blocks.
  const uint8_t colw=32,rowh=32;
  const uint16_t blocksize=colw*rowh;
  uint8_t colc=image->w/colw;
  uint8_t rowc=image->h/rowh;
  int16_t blockxstride=(colw*pixelsize)>>3;
  int16_t blockystride=rowh*image->stride;
  uint8_t *blockrowp=image->v;
  //TODO Currently this will only work for pixels up to 8 bits.
  int8_t shift0=0;
  if (pixelsize<8) shift0=8-pixelsize;
  uint8_t pixelmask=0xff;
  if (pixelsize<8) pixelmask=(1<<pixelsize)-1;
  for (;rowc-->0;blockrowp+=blockystride) {
    uint8_t *blockp=blockrowp;
    uint8_t xi=colc;
    for (;xi-->0;blockp+=blockxstride) {
    
      // Within each block, walk back and forth in diagonal strips, while stepping straight along the mask.
      uint8_t d=0; // 0=(x++,y--), 1=(x--,y++)
      uint8_t subx=0,suby=0;
      uint8_t *subp=blockp;
      int8_t shift=shift0;
      uint16_t i=blocksize;
      for (;i-->0;maskp++) {
        if (maskp>=sizeof(mask)) maskp=0;
        if (mask[maskp]) *subp&=~(pixelmask<<shift);
        
        if (d) {
          if (!subx||(suby==rowh-1)) {
            d=0;
            if (suby<rowh-1) { suby++; subp+=image->stride; }
            else { subx++; shift-=pixelsize; if (shift<0) { shift+=8; subp++; } }
          } else {
            subx--; suby++;
            shift+=pixelsize; if (shift>=8) { shift-=8; subp--; }
            subp+=image->stride;
          }
        } else {
          if (!suby||(subx==colw-1)) {
            d=1;
            if (subx<colw-1) { subx++; shift-=pixelsize; if (shift<0) { shift+=8; subp++; } }
            else { suby++; subp+=image->stride; }
          } else {
            subx++; suby--;
            shift-=pixelsize; if (shift<0) { shift+=8; subp++; }
            subp-=image->stride;
          }
        }
      }
    }
  }
}
 
static void fmn_image_transition_dissolve(
  struct fmn_image *to,
  const struct fmn_image *from,
  uint8_t p,uint8_t c
) {
  if (c<2) return;
  uint8_t midc=c>>1;
  if (p<midc) {
    fmn_image_dissolve_blackout(to,p,midc);
  } else {
    fmn_image_blit(to,0,0,from,0,0,to->w,to->h,0);
    fmn_image_dissolve_blackout(to,c-p,midc);
  }
}

/* Spotlight.
 */
 
static void fmn_image_spotlight_1(
  struct fmn_image *image,
  int16_t x,int16_t y,
  uint8_t p,uint8_t c
) {

  // Force (x,y) inside the image, and not on its edges either.
  if (x<1) x=1; else if (x>=image->w-1) x=image->w-2;
  if (y<1) y=1; else if (y>=image->h-1) y=image->h-2;

  // The Box Strategy. Blackout 4 rectangles, leaving one whose edges are a proportionate distance from each screen edge.
  // This is wicked simple to implement... does it look ok?
  if (1) {
    int16_t left=(x*p)/c;
    int16_t top=(y*p)/c;
    int16_t right=image->w-((image->w-x)*p)/c-1;
    int16_t bottom=image->h-((image->h-y)*p)/c-1;
    fmn_image_fill_rect(image,0,0,left,image->h,0);
    fmn_image_fill_rect(image,right,0,image->w,image->h,0);
    fmn_image_fill_rect(image,left,0,right-left,top,0);
    fmn_image_fill_rect(image,left,bottom,right-left,image->h,0);
  }
}
 
static void fmn_image_transition_spotlight(
  struct fmn_image *to,
  const struct fmn_image *from,
  const struct fmn_transition *transition
) {
  // Knock out edge cases and decide which half we're drawing.
  uint8_t midc=transition->c>>1;
  if (transition->p>=transition->c) return;
  if (transition->p==0) {
    fmn_image_blit(to,0,0,from,0,0,to->w,to->h,0);
    return;
  }
  if (transition->p==midc) {
    fmn_image_fill_rect(to,0,0,to->w,to->h,0);
    return;
  }
  if (transition->p<midc) {
    fmn_image_blit(to,0,0,from,0,0,to->w,to->h,0);
    fmn_image_spotlight_1(to,transition->outx,transition->outy,transition->p,midc);
  } else {
    fmn_image_spotlight_1(to,transition->inx,transition->iny,transition->c-transition->p,midc);
  }
}

/* Perform transition, public entry point.
 */

void fmn_image_transition(
  struct fmn_image *to,
  const struct fmn_image *from,
  struct fmn_transition *transition
) {
  // All three args are required; if they left out (transition), don't try to guess anything.
  if (!to||!from||!transition) return;
  
  // With (p) on one of its edges, the image is either fully "to" or "from".
  // These rules also ensure that (c>0).
  if (transition->p>=transition->c) {
    return;
  }
  if (!transition->p) {
    fmn_image_blit(to,0,0,from,0,0,from->w,from->h,0);
    return;
  }
  
  // Since we operate on the "to" image, it makes more sense to reverse (p).
  // Now p==0 means do nothing.
  uint8_t p=transition->c-transition->p;
  
  switch (transition->mode) {
    case FMN_TRANSITION_PAN_LEFT: fmn_image_transition_pan_left(to,from,p,transition->c); return;
    case FMN_TRANSITION_PAN_RIGHT: fmn_image_transition_pan_right(to,from,p,transition->c); return;
    case FMN_TRANSITION_PAN_UP: fmn_image_transition_pan_up(to,from,p,transition->c); return;
    case FMN_TRANSITION_PAN_DOWN: fmn_image_transition_pan_down(to,from,p,transition->c); return;
    case FMN_TRANSITION_DISSOLVE: fmn_image_transition_dissolve(to,from,p,transition->c); return;
    case FMN_TRANSITION_SPOTLIGHT: fmn_image_transition_spotlight(to,from,transition); return;
  }
  
  // Anything unknown, use the final frame.
  fmn_image_blit(to,0,0,from,0,0,from->w,from->h,0);
}
