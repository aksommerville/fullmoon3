#include "fmn_hero_internal.h"

/* Init.
 */
 
static void _hero_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
}

/* Update.
 */
 
static void _hero_update(struct fmn_sprite *sprite) {
  // fmn_hero_update() should be called separately, before updating sprites.
  // Our job here is just to ensure that the sprite is in sync with the global hero.
  sprite->x=fmn_hero.x;
  sprite->y=fmn_hero.y;
}

/* Render while riding broom.
 */
 
static void fmn_hero_render_broom(struct fmn_image *fb,int16_t x,int16_t y) {
  //TODO
  int16_t dstx=x-FMN_TILESIZE;
  int16_t dsty=y-FMN_TILESIZE-((FMN_TILESIZE*4)/8);
  if (fmn_hero.renderseq&1) {
    fmn_image_fill_rect(fb,x-(FMN_TILESIZE>>1),dsty+(FMN_TILESIZE<<1)-1,FMN_TILESIZE,1,0);
  }
  fmn_image_blit(fb,dstx,dsty,&fmnr_image_hero,48,40,FMN_TILESIZE<<1,FMN_TILESIZE<<1,(fmn_hero.stickydx<0)?FMN_XFORM_XREV:0);
}

/* Render while using wand.
 */
 
static void fmn_hero_render_wand(struct fmn_image *fb,int16_t x,int16_t y) {
  //TODO
}

/* Render while using violin.
 */
 
static void fmn_hero_render_violin(struct fmn_image *fb,int16_t x,int16_t y) {
  //TODO
}

/* Render carried item, 2 tiles wide.
 */
 
static void fmn_hero_render_carry_wide(struct fmn_image *fb,int16_t x,int16_t y,uint8_t tileid) {
  switch (fmn_hero.facedir) {
    case FMN_DIR_E: {
        fmn_image_blit_tile(fb,x-(FMN_TILESIZE>>1),y,&fmnr_image_hero,tileid,0);
        fmn_image_blit_tile(fb,x+(FMN_TILESIZE>>1),y,&fmnr_image_hero,tileid+1,0);
      } break;
    case FMN_DIR_W: {
        fmn_image_blit_tile(fb,x-(FMN_TILESIZE>>1),y,&fmnr_image_hero,tileid+1,FMN_XFORM_XREV);
        fmn_image_blit_tile(fb,x+(FMN_TILESIZE>>1),y,&fmnr_image_hero,tileid,FMN_XFORM_XREV);
      } break;
    case FMN_DIR_N: {
        fmn_image_blit_tile(fb,x+(FMN_TILESIZE>>1),y-FMN_TILESIZE,&fmnr_image_hero,tileid+0x10,0);
        fmn_image_blit_tile(fb,x+(FMN_TILESIZE>>1),y,&fmnr_image_hero,tileid+0x20,0);
      } break;
    case FMN_DIR_S: {
        fmn_image_blit_tile(fb,x-(FMN_TILESIZE>>1),y-FMN_TILESIZE,&fmnr_image_hero,tileid+0x10,0);
        fmn_image_blit_tile(fb,x-(FMN_TILESIZE>>1),y,&fmnr_image_hero,tileid+0x20,0);
      } break;
  }
}

/* Render.
 */
 
static void _hero_render(struct fmn_image *fb,struct fmn_sprite *sprite,int16_t x,int16_t y) {
  const struct fmn_image *src=&fmnr_image_hero;
  fmn_hero.renderseq++;
  
  // Actions that replace the whole thing while in progress.
  switch (fmn_hero.action) {
    case FMN_ITEM_broom: fmn_hero_render_broom(fb,x,y); return;
    case FMN_ITEM_wand: fmn_hero_render_wand(fb,x,y); return;
    case FMN_ITEM_violin: fmn_hero_render_violin(fb,x,y); return;
  }
  
  // Body.
  uint8_t bodyframe;
  switch (fmn_hero.animframe) {
    case 0: case 2: bodyframe=0; break;
    case 1: bodyframe=1; break;
    case 3: bodyframe=2; break;
  }
  if (fmn_hero.facedir==FMN_DIR_N) fmn_image_blit_tile(fb,x,y,src,0x31+bodyframe,0);
  else if (fmn_hero.facedir==FMN_DIR_W) fmn_image_blit_tile(fb,x,y,src,0x21+bodyframe,FMN_XFORM_XREV);
  else fmn_image_blit_tile(fb,x,y,src,0x21+bodyframe,0);
  
  // Head.
  int16_t dsty=y-((FMN_TILESIZE*5)>>3);
  switch (fmn_hero.facedir) {
    case FMN_DIR_N: fmn_image_blit_tile(fb,x,dsty,src,0x12,0); break;
    case FMN_DIR_S: fmn_image_blit_tile(fb,x,dsty,src,0x13,0); break;
    case FMN_DIR_W: fmn_image_blit_tile(fb,x,dsty,src,0x11,FMN_XFORM_XREV); break;
    case FMN_DIR_E: fmn_image_blit_tile(fb,x,dsty,src,0x11,0); break;
  }
  
  // Item.
  uint8_t itemid=fmn_state_get_selected_item_if_possessed();
  switch (itemid) {
    case FMN_ITEM_broom: fmn_hero_render_carry_wide(fb,x,y,0x40); break;
    case FMN_ITEM_feather:break;
    case FMN_ITEM_wand:break;
    case FMN_ITEM_violin:break;
    case FMN_ITEM_bell:break;
    case FMN_ITEM_chalk:break;
    case FMN_ITEM_pitcher:break;
    case FMN_ITEM_coin:break;
    case FMN_ITEM_match:break;
    case FMN_ITEM_corn:break;
    case FMN_ITEM_umbrella: fmn_hero_render_carry_wide(fb,x,y,0x42); break;
    case FMN_ITEM_shovel: fmn_hero_render_carry_wide(fb,x,y,0x44); break;
    case FMN_ITEM_compass:break;
  }
  
  // Hat.
  int16_t dstx=x-FMN_TILESIZE;
  dsty=y-((FMN_TILESIZE*10)>>3)-(FMN_TILESIZE>>1);
  switch (fmn_hero.facedir) {
    case FMN_DIR_S: case FMN_DIR_E: fmn_image_blit(fb,dstx-1,dsty,src,0,0,FMN_TILESIZE<<1,FMN_TILESIZE,0); break;
    default: fmn_image_blit(fb,dstx,dsty,src,0,0,FMN_TILESIZE<<1,FMN_TILESIZE,0);
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_hero={
  .name="hero",
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};
