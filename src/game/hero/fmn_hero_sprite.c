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

/* Render.
 */
 
static void _hero_render(struct fmn_image *fb,struct fmn_sprite *sprite,int16_t x,int16_t y) {
  const struct fmn_image *src=&fmnr_image_hero;
  
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
