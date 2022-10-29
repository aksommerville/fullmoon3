#include "fmn_hero_internal.h"

/* Init.
 */
 
static void _hero_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->flags=FMN_SPRITE_FLAG_SOLID;
}

/* Update.
 */
 
void fmn_hero_sprite_update(struct fmn_sprite *sprite) {
  // fmn_hero_update() should be called separately, before updating sprites.
  // Our job here is just to ensure that the sprite is in sync with the global hero.
  sprite->x=fmn_hero.x;
  sprite->y=fmn_hero.y;
}

/* Render while riding broom.
 */
 
static void fmn_hero_render_broom(struct fmn_image *fb,int16_t x,int16_t y) {
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

  // Body, head, hat: Same as if we were facing south in the default pose.
  fmn_image_blit_tile(fb,x,y,&fmnr_image_hero,0x21,0);
  int16_t dsty=y-((FMN_TILESIZE*5)>>3);
  fmn_image_blit_tile(fb,x,dsty,&fmnr_image_hero,0x13,0);
  int16_t dstx=x-FMN_TILESIZE;
  dsty=y-((FMN_TILESIZE*10)>>3)-(FMN_TILESIZE>>1);
  fmn_image_blit(fb,dstx-1,dsty,&fmnr_image_hero,0,0,FMN_TILESIZE<<1,FMN_TILESIZE,0);
  
  // Hand.
  switch (fmn_hero.wanddir) {
    case FMN_DIR_W: fmn_image_blit_tile(fb,x-(FMN_TILESIZE*7)/8,y,&fmnr_image_hero,0x4a,FMN_XFORM_XREV); break;
    case FMN_DIR_E: fmn_image_blit_tile(fb,x+(FMN_TILESIZE*7)/8,y,&fmnr_image_hero,0x4a,0); break;
    case FMN_DIR_N: fmn_image_blit_tile(fb,x,y-(FMN_TILESIZE*6)/8,&fmnr_image_hero,0x4b,FMN_XFORM_YREV); break;
    case FMN_DIR_S: fmn_image_blit_tile(fb,x,y+(FMN_TILESIZE*4)/8,&fmnr_image_hero,0x4b,0); break;
    default: fmn_image_blit_tile(fb,x,y,&fmnr_image_hero,0x49,0);
  }
}

/* Render while using violin.
 */
 
static void fmn_hero_render_violin(struct fmn_image *fb,int16_t x,int16_t y) {

  // Body, head, hat: Same as if we were facing south in the default pose.
  fmn_image_blit_tile(fb,x,y,&fmnr_image_hero,0x21,0);
  int16_t dsty=y-((FMN_TILESIZE*5)>>3);
  fmn_image_blit_tile(fb,x,dsty,&fmnr_image_hero,0x13,0);
  int16_t dstx=x-FMN_TILESIZE;
  dsty=y-((FMN_TILESIZE*10)>>3)-(FMN_TILESIZE>>1);
  fmn_image_blit(fb,dstx-1,dsty,&fmnr_image_hero,0,0,FMN_TILESIZE<<1,FMN_TILESIZE,0);
  
  fmn_image_blit_tile(fb,x+(FMN_TILESIZE*5)/8,y-(FMN_TILESIZE*2)/8,&fmnr_image_hero,0x59,0);
  
  if (fmn_hero.violindir) {
    uint8_t frame=fmn_hero.animframe;
    if (frame==3) frame=1;
    dstx=x+(FMN_TILESIZE*4)/8-frame;
    dsty=y-(FMN_TILESIZE*5)/8+frame;
    fmn_image_blit_tile(fb,dstx,dsty,&fmnr_image_hero,0x5a,0);
  } else {
    fmn_image_blit_tile(fb,x-(FMN_TILESIZE*5)/8,y-(FMN_TILESIZE*1)/8,&fmnr_image_hero,0x69,0);
  }
}

/* Feather.
 */
 
static void fmn_hero_render_feather_active(struct fmn_image *fb,int16_t x,int16_t y) {
  const int16_t period=40;
  const int16_t range=(FMN_TILESIZE*2)/8;
  int16_t phase=fmn_hero.actiontime%period;
  uint8_t frame;
  switch ((phase*4)/period) {
    case 0: frame=2; break;
    case 1: frame=1; break;
    case 2: frame=0; break;
    default: frame=1; break;
  }
  if (phase>=period>>1) phase=period-phase;
  int16_t displacement=(phase*range*2+(period>>1))/period-range;
  switch (fmn_hero.facedir) {
    case FMN_DIR_E: fmn_image_blit_tile(fb,x+((FMN_TILESIZE*6)/8),y+((FMN_TILESIZE*1)/8)+displacement,&fmnr_image_hero,0x48+frame*0x10,0); break;
    case FMN_DIR_W: fmn_image_blit_tile(fb,x-((FMN_TILESIZE*6)/8),y+((FMN_TILESIZE*1)/8)+displacement,&fmnr_image_hero,0x48+frame*0x10,FMN_XFORM_XREV); break;
    case FMN_DIR_N: fmn_image_blit_tile(fb,x+((FMN_TILESIZE*4)/8)+displacement,y-((FMN_TILESIZE*5)/8),&fmnr_image_hero,0x76+frame,FMN_XFORM_YREV); break;
    case FMN_DIR_S: fmn_image_blit_tile(fb,x-((FMN_TILESIZE*1)/8)+displacement,y+((FMN_TILESIZE*4)/8),&fmnr_image_hero,0x76+frame,0); break;
  }
}

/* Render carried item.
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

static void fmn_hero_render_carry_small(struct fmn_image *fb,int16_t x,int16_t y,uint8_t tileid) {
  switch (fmn_hero.facedir) {
    case FMN_DIR_E: fmn_image_blit_tile(fb,x+((FMN_TILESIZE*6)/8),y,&fmnr_image_hero,tileid,0); break;
    case FMN_DIR_W: fmn_image_blit_tile(fb,x-((FMN_TILESIZE*6)/8),y,&fmnr_image_hero,tileid,FMN_XFORM_XREV); break;
    case FMN_DIR_N: fmn_image_blit_tile(fb,x+((FMN_TILESIZE*4)/8),y-((FMN_TILESIZE*5)/8),&fmnr_image_hero,tileid+0x10,FMN_XFORM_YREV); break;
    case FMN_DIR_S: fmn_image_blit_tile(fb,x-((FMN_TILESIZE*2)/8),y+((FMN_TILESIZE*4)/8),&fmnr_image_hero,tileid+0x10,0); break;
  }
}

/* Render item.
 */
 
static void fmn_hero_render_item(struct fmn_image *fb,int16_t x,int16_t y) {
  if (fmn_hero.action>=0) switch (fmn_hero.action) {
    case FMN_ITEM_feather: fmn_hero_render_feather_active(fb,x,y); break;
  } else {
    uint8_t itemid=fmn_state_get_selected_item_if_possessed();
    switch (itemid) {
      case FMN_ITEM_broom: fmn_hero_render_carry_wide(fb,x,y,0x40); break;
      case FMN_ITEM_feather: fmn_hero_render_carry_small(fb,x,y,0x51); break;
      case FMN_ITEM_wand: fmn_hero_render_carry_small(fb,x,y,0x53); break;
      case FMN_ITEM_violin: fmn_hero_render_carry_small(fb,x,y,0x55); break;
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
  
  // Item renders before body when facing north only.
  if (fmn_hero.facedir==FMN_DIR_N) {
    fmn_hero_render_item(fb,x,y);
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
  
  // Item, facing anywhere but north.
  if (fmn_hero.facedir!=FMN_DIR_N) {
    fmn_hero_render_item(fb,x,y);
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
  .update=fmn_hero_sprite_update,
  .render=_hero_render,
};
