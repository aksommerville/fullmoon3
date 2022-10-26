#include "fmn_pause_internal.h"

// Items will lay out in a 5x3 grid.
#define FMN_PAUSE_COLC 5
#define FMN_PAUSE_ROWC 3

/* Init.
 * Called every time we enter the pause mode.
 */
 
void fmn_game_mode_pause_init() {
}

/* Move cursor.
 */
 
static void fmn_pause_move_cursor(int8_t dx,int8_t dy) {
  uint8_t itemid=fmn_state_get_selected_item();
  int8_t col=itemid%FMN_PAUSE_COLC;
  int8_t row=itemid/FMN_PAUSE_COLC;
  col+=dx;
  if (col<0) col=FMN_PAUSE_COLC-1;
  else if (col>=FMN_PAUSE_COLC) col=0;
  row+=dy;
  if (row<0) row=FMN_PAUSE_ROWC-1;
  else if (row>=FMN_PAUSE_ROWC) row=0;
  itemid=row*FMN_PAUSE_COLC+col;
  fmn_state_set_selected_item(itemid);
}

/* Receive input.
 */
 
void fmn_game_mode_pause_input(uint8_t input,uint8_t pvinput) {

  // Pressing either button returns to play mode.
  if (
    ((input&FMN_BUTTON_A)&&!(pvinput&FMN_BUTTON_A))||
    ((input&FMN_BUTTON_B)&&!(pvinput&FMN_BUTTON_B))
  ) {
    fmn_game_mode_set_play();
    return;
  }
  
  // Dpad moves our cursor, "on" events only.
  if ((input&FMN_BUTTON_LEFT)&&!(pvinput&FMN_BUTTON_LEFT)) fmn_pause_move_cursor(-1,0);
  if ((input&FMN_BUTTON_RIGHT)&&!(pvinput&FMN_BUTTON_RIGHT)) fmn_pause_move_cursor(1,0);
  if ((input&FMN_BUTTON_UP)&&!(pvinput&FMN_BUTTON_UP)) fmn_pause_move_cursor(0,-1);
  if ((input&FMN_BUTTON_DOWN)&&!(pvinput&FMN_BUTTON_DOWN)) fmn_pause_move_cursor(0,1);
}

/* Update.
 */
 
void fmn_game_mode_pause_update() {
}

/* Render.
 */
 
static uint8_t fmn_pause_anim_clock=0;
static uint8_t fmn_pause_anim_frame=0;
 
static void fmn_pause_render_cursor(struct fmn_image *fb,int16_t dstx,int16_t dsty) {
  if (fmn_pause_anim_clock) fmn_pause_anim_clock=0;
  else {
    fmn_pause_anim_clock=8;
    if (++fmn_pause_anim_frame>=8) fmn_pause_anim_frame=0;
  }
  int16_t srcy=112+fmn_pause_anim_frame*2;
  fmn_image_blit(fb,dstx+ 2,dsty   ,&fmnr_image_uibits,48,srcy,8,2,0);
  fmn_image_blit(fb,dstx+10,dsty   ,&fmnr_image_uibits,48,srcy,8,2,0);
  fmn_image_blit(fb,dstx+ 2,dsty+18,&fmnr_image_uibits,48,srcy,8,2,FMN_XFORM_XREV|FMN_XFORM_YREV);
  fmn_image_blit(fb,dstx+10,dsty+18,&fmnr_image_uibits,48,srcy,8,2,FMN_XFORM_XREV|FMN_XFORM_YREV);
  int16_t srcx=56+(fmn_pause_anim_frame&3)*2;
  srcy=(fmn_pause_anim_frame&4)?120:112;
  fmn_image_blit(fb,dstx   ,dsty+ 2,&fmnr_image_uibits,srcx,srcy,2,8,0);
  fmn_image_blit(fb,dstx   ,dsty+10,&fmnr_image_uibits,srcx,srcy,2,8,0);
  fmn_image_blit(fb,dstx+18,dsty+ 2,&fmnr_image_uibits,srcx,srcy,2,8,FMN_XFORM_YREV|FMN_XFORM_XREV);
  fmn_image_blit(fb,dstx+18,dsty+10,&fmnr_image_uibits,srcx,srcy,2,8,FMN_XFORM_YREV|FMN_XFORM_XREV);
}

static void fmn_pause_render_count(struct fmn_image *fb,int16_t dstx,int16_t dsty,uint8_t v) {
  if (v>=100) fmn_image_fill_rect(fb,dstx+5,dsty+11,13,7,3);
  else if (v>=10) fmn_image_fill_rect(fb,dstx+9,dsty+11,9,7,3);
  else fmn_image_fill_rect(fb,dstx+13,dsty+11,5,7,3);
  dstx+=14;
  dsty+=12;
  fmn_image_blit(fb,dstx,dsty,&fmnr_image_uibits,(v%10)*3,9,3,5,0);
  if (v>=10) {
    dstx-=4;
    fmn_image_blit(fb,dstx,dsty,&fmnr_image_uibits,((v/10)%10)*3,9,3,5,0);
    if (v>=100) {
      dstx-=4;
      fmn_image_blit(fb,dstx,dsty,&fmnr_image_uibits,((v/100)%10)*3,9,3,5,0);
    }
  }
}
 
void fmn_game_mode_pause_render(struct fmn_image *fb) {
  fmn_image_fill_rect(fb,0,0,fb->w,fb->h,0);
  
  int16_t dstx0=(fb->w>>1)-((FMN_PAUSE_COLC*18+2)>>1);
  int16_t dsty=(fb->h>>1)-((FMN_PAUSE_ROWC*18+2)>>1);
  uint16_t items=fmn_state_get_possessed_items();
  uint8_t selection=fmn_state_get_selected_item();
  uint16_t mask=1;
  uint8_t itemid=0;
  uint8_t yi=FMN_PAUSE_ROWC;
  for (;yi-->0;dsty+=18) {
    int16_t dstx=dstx0;
    uint8_t xi=FMN_PAUSE_COLC;
    for (;xi-->0;dstx+=18,mask<<=1,itemid++) {
      if (items&mask) {
        int16_t srcx=(itemid&3)*16;
        int16_t srcy=64+(itemid>>2)*16;
        fmn_image_blit(fb,dstx+2,dsty+2,&fmnr_image_uibits,srcx,srcy,16,16,0);
        uint8_t count=fmn_state_get_item_count(itemid);
        if (count<0xff) {
          if (itemid==FMN_ITEM_pitcher) {
            if ((count>0)&&(count<=4)) {
              fmn_image_blit(fb,dstx+5,dsty+9,&fmnr_image_uibits,65,65+(count-1)*8,7,7,0);
            }
          } else {
            fmn_pause_render_count(fb,dstx,dsty,count);
          }
        }
      }
      if (itemid==selection) {
        fmn_pause_render_cursor(fb,dstx,dsty);
      }
    }
  }
}
