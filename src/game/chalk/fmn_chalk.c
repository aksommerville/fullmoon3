#include "api/fmn_platform.h"
#include "game/fmn_game_mode.h"
#include "game/image/fmn_image.h"
#include "game/sprite/fmn_sprite.h"
#include "game/fmn_data.h"
#include <stdio.h>

/* Globals.
 */
 
static uint32_t fmn_chalk_bits=0; // relevant: 0x000fffff
static int8_t fmn_chalk_cursorx,fmn_chalk_cursory; // 0..2
static int8_t fmn_chalk_anchorx,fmn_chalk_anchory; // '' or <0 if button not held
static uint16_t fmn_chalk_renderseq; // for animation
static struct fmn_sprite *fmn_chalk_board=0;
 
/* Init.
 */
 
void fmn_game_mode_chalk_init(struct fmn_sprite *chalkboard) {
  fmn_chalk_bits=0;
  if (fmn_chalk_board=chalkboard) {
    fmn_chalk_bits=fmn_chalk_board->bv[0]|(fmn_chalk_board->bv[1]<<8)|(fmn_chalk_board->bv[2]<<16);
  }
  fmn_chalk_cursorx=fmn_chalk_cursory=1;
  fmn_chalk_anchorx=fmn_chalk_anchory=-1;
  fmn_chalk_renderseq=0;
}

/* Move cursor.
 */
 
static void fmn_chalk_clamp_cursor(int8_t *cursor,int8_t anchor) {
  if (anchor>=0) {
    if (*cursor<anchor-1) *cursor=anchor-1;
    else if (*cursor>anchor+1) *cursor=anchor+1;
  }
  if (*cursor<0) *cursor=0;
  else if (*cursor>2) *cursor=2;
}
 
static void fmn_chalk_move(int8_t dx,int8_t dy) {
  fmn_chalk_cursorx+=dx;
  fmn_chalk_cursory+=dy;
  fmn_chalk_clamp_cursor(&fmn_chalk_cursorx,fmn_chalk_anchorx);
  fmn_chalk_clamp_cursor(&fmn_chalk_cursory,fmn_chalk_anchory);
}

/* Commit line and disengage anchor.
 * (I'm trying not to say "drop anchor", that would be confusing).
 */
 
static void fmn_chalk_commit() {
  if (fmn_chalk_anchorx<0) return;
  if ((fmn_chalk_anchorx!=fmn_chalk_cursorx)||(fmn_chalk_anchory!=fmn_chalk_cursory)) {
    // Reduce cursor and anchor each to a scalar 0..8.
    uint8_t cursor=fmn_chalk_cursory*3+fmn_chalk_cursorx;
    uint8_t anchor=fmn_chalk_anchory*3+fmn_chalk_anchorx;
    // Make cursor the lower index. They're symmetric, this is just for convenience.
    if (cursor>anchor) {
      uint8_t tmp=cursor;
      cursor=anchor;
      anchor=tmp;
    }
    // Now we can combine them in a kind of readable* list...
    // [*] i read weird shit
    switch ((cursor<<4)|anchor) {
      case 0x01: fmn_chalk_bits^=1<<19; break; // NW:N
      case 0x03: fmn_chalk_bits^=1<<17; break; // NW:W
      case 0x04: fmn_chalk_bits^=1<<16; break; // NW:MID
      case 0x12: fmn_chalk_bits^=1<<18; break; // N:NE
      case 0x13: fmn_chalk_bits^=1<<15; break; // N:W
      case 0x14: fmn_chalk_bits^=1<<14; break; // N:MID
      case 0x15: fmn_chalk_bits^=1<<13; break; // N:E
      case 0x24: fmn_chalk_bits^=1<<12; break; // NE:MID
      case 0x25: fmn_chalk_bits^=1<<11; break; // NE:E
      case 0x34: fmn_chalk_bits^=1<<10; break; // W:MID
      case 0x36: fmn_chalk_bits^=1<< 8; break; // W:SW
      case 0x37: fmn_chalk_bits^=1<< 7; break; // W:S
      case 0x45: fmn_chalk_bits^=1<< 9; break; // MID:E
      case 0x46: fmn_chalk_bits^=1<< 6; break; // MID:SW
      case 0x47: fmn_chalk_bits^=1<< 5; break; // MID:S
      case 0x48: fmn_chalk_bits^=1<< 4; break; // MID:SE
      case 0x57: fmn_chalk_bits^=1<< 3; break; // E:S
      case 0x58: fmn_chalk_bits^=1<< 2; break; // E:SE
      case 0x67: fmn_chalk_bits^=1<< 1; break; // SW:S
      case 0x78: fmn_chalk_bits^=1<< 0; break; // S:SE
    }
    if (fmn_chalk_board) {
      fmn_chalk_board->bv[0]=fmn_chalk_bits;
      fmn_chalk_board->bv[1]=fmn_chalk_bits>>8;
      fmn_chalk_board->bv[2]=fmn_chalk_bits>>16;
    }
  }
  fmn_chalk_anchorx=-1;
  fmn_chalk_anchory=-1;
}

/* Input.
 */
 
void fmn_game_mode_chalk_input(uint8_t input,uint8_t pvinput) {
  
  // Dpad to move cursor. "on" events only.
  if ((input&FMN_BUTTON_LEFT )&&!(pvinput&FMN_BUTTON_LEFT )) fmn_chalk_move(-1, 0);
  if ((input&FMN_BUTTON_RIGHT)&&!(pvinput&FMN_BUTTON_RIGHT)) fmn_chalk_move( 1, 0);
  if ((input&FMN_BUTTON_UP   )&&!(pvinput&FMN_BUTTON_UP   )) fmn_chalk_move( 0,-1);
  if ((input&FMN_BUTTON_DOWN )&&!(pvinput&FMN_BUTTON_DOWN )) fmn_chalk_move( 0, 1);
  
  // A on to engage the anchor.
  if ((input&FMN_BUTTON_A)&&!(pvinput&FMN_BUTTON_A)) {
    fmn_chalk_anchorx=fmn_chalk_cursorx;
    fmn_chalk_anchory=fmn_chalk_cursory;
  // A off to commit the line, if there is one.
  } else if (!(input&FMN_BUTTON_A)&&(pvinput&FMN_BUTTON_A)) {
    fmn_chalk_commit();
  }
  
  // B to return to game.
  if ((input&FMN_BUTTON_B)&&!(pvinput&FMN_BUTTON_B)) {
    fmn_game_mode_set_play();
  }
}

/* Render.
 */
 
void fmn_game_mode_chalk_render(struct fmn_image *fb) {
  fmn_chalk_renderseq++;
  fmn_image_fill_rect(fb,0,0,fb->w,fb->h,0);
  
  const int16_t narrow=5;
  const int16_t wide=15;
  const int16_t field=narrow*3+wide*2;
  int16_t boundsl=(fb->w>>1)-(field>>1);
  int16_t boundst=(fb->h>>1)-(field>>1);
  
  // Cheat a TRANSPARENT flag onto uibits.
  uint8_t pvflags=fmnr_image_uibits.flags;
  fmnr_image_uibits.flags|=FMN_IMAGE_FLAG_TRANSPARENT;
  
  // Draw the nine anchor points.
  uint8_t ay=0; for (;ay<3;ay++) {
    int16_t dsty=boundst+ay*(narrow+wide);
    uint8_t ax=0; for (;ax<3;ax++) {
      int16_t dstx=boundsl+ax*(narrow+wide);
      if ((ax==fmn_chalk_cursorx)&&(ay==fmn_chalk_cursory)) {
        if ((fmn_chalk_anchorx<0)||(fmn_chalk_renderseq%10<5)) {
          fmn_image_blit(fb,dstx,dsty,&fmnr_image_uibits,5,19,narrow,narrow,0);
        }
      } else if ((ax==fmn_chalk_anchorx)&&(ay==fmn_chalk_anchory)) {
        if (fmn_chalk_renderseq%10<5) {
          fmn_image_blit(fb,dstx,dsty,&fmnr_image_uibits,0,19,narrow,narrow,0);
        }
      } else {
        fmn_image_blit(fb,dstx,dsty,&fmnr_image_uibits,0,19,narrow,narrow,0);
      }
    }
  }
  
  // Up to 6 horizontal bars.
  if (fmn_chalk_bits&0x00080000) fmn_image_blit(fb,boundsl+narrow,       boundst+(narrow+wide)*0,&fmnr_image_uibits,10,19,wide,narrow,0);
  if (fmn_chalk_bits&0x00040000) fmn_image_blit(fb,boundsl+wide+narrow*2,boundst+(narrow+wide)*0,&fmnr_image_uibits,10,19,wide,narrow,0);
  if (fmn_chalk_bits&0x00000400) fmn_image_blit(fb,boundsl+narrow,       boundst+(narrow+wide)*1,&fmnr_image_uibits,10,19,wide,narrow,0);
  if (fmn_chalk_bits&0x00000200) fmn_image_blit(fb,boundsl+wide+narrow*2,boundst+(narrow+wide)*1,&fmnr_image_uibits,10,19,wide,narrow,0);
  if (fmn_chalk_bits&0x00000002) fmn_image_blit(fb,boundsl+narrow,       boundst+(narrow+wide)*2,&fmnr_image_uibits,10,19,wide,narrow,0);
  if (fmn_chalk_bits&0x00000001) fmn_image_blit(fb,boundsl+wide+narrow*2,boundst+(narrow+wide)*2,&fmnr_image_uibits,10,19,wide,narrow,0);
  
  // Up to 6 vertical bars.
  if (fmn_chalk_bits&0x00020000) fmn_image_blit(fb,boundsl+(wide+narrow)*0,boundst+narrow       ,&fmnr_image_uibits,30,24,narrow,wide,0);
  if (fmn_chalk_bits&0x00000100) fmn_image_blit(fb,boundsl+(wide+narrow)*0,boundst+wide+narrow*2,&fmnr_image_uibits,30,24,narrow,wide,0);
  if (fmn_chalk_bits&0x00004000) fmn_image_blit(fb,boundsl+(wide+narrow)*1,boundst+narrow       ,&fmnr_image_uibits,30,24,narrow,wide,0);
  if (fmn_chalk_bits&0x00000020) fmn_image_blit(fb,boundsl+(wide+narrow)*1,boundst+wide+narrow*2,&fmnr_image_uibits,30,24,narrow,wide,0);
  if (fmn_chalk_bits&0x00000800) fmn_image_blit(fb,boundsl+(wide+narrow)*2,boundst+narrow       ,&fmnr_image_uibits,30,24,narrow,wide,0);
  if (fmn_chalk_bits&0x00000004) fmn_image_blit(fb,boundsl+(wide+narrow)*2,boundst+wide+narrow*2,&fmnr_image_uibits,30,24,narrow,wide,0);
  
  // Up to 8 diagonal bars.
  if (fmn_chalk_bits&0x00010000) fmn_image_blit(fb,boundsl+narrow            ,boundst+narrow            ,&fmnr_image_uibits, 0,24,wide,wide,0);
  if (fmn_chalk_bits&0x00002000) fmn_image_blit(fb,boundsl+narrow+wide+narrow,boundst+narrow            ,&fmnr_image_uibits, 0,24,wide,wide,0);
  if (fmn_chalk_bits&0x00000080) fmn_image_blit(fb,boundsl+narrow            ,boundst+narrow+wide+narrow,&fmnr_image_uibits, 0,24,wide,wide,0);
  if (fmn_chalk_bits&0x00000010) fmn_image_blit(fb,boundsl+narrow+wide+narrow,boundst+narrow+wide+narrow,&fmnr_image_uibits, 0,24,wide,wide,0);
  if (fmn_chalk_bits&0x00008000) fmn_image_blit(fb,boundsl+narrow            ,boundst+narrow            ,&fmnr_image_uibits,15,24,wide,wide,0);
  if (fmn_chalk_bits&0x00001000) fmn_image_blit(fb,boundsl+narrow+wide+narrow,boundst+narrow            ,&fmnr_image_uibits,15,24,wide,wide,0);
  if (fmn_chalk_bits&0x00000040) fmn_image_blit(fb,boundsl+narrow            ,boundst+narrow+wide+narrow,&fmnr_image_uibits,15,24,wide,wide,0);
  if (fmn_chalk_bits&0x00000008) fmn_image_blit(fb,boundsl+narrow+wide+narrow,boundst+narrow+wide+narrow,&fmnr_image_uibits,15,24,wide,wide,0);
  
  fmnr_image_uibits.flags=pvflags;
}
