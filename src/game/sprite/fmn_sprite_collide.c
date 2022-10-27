#include "fmn_sprite.h"
#include "game/map/fmn_map.h"
#include "game/play/fmn_play.h"
#include "game/image/fmn_image.h"
#include <stdio.h>

/* Map.
 */
 
static int fmn_sprite_collide_map(
  int16_t x,int16_t y,int16_t w,int16_t h,
  const uint8_t *cellv,const uint8_t *propv,
  uint8_t features
) {
  int16_t cola=x/FMN_MM_PER_TILE; if (cola<0) cola=0;
  int16_t colz=(x+w-1)/FMN_MM_PER_TILE; if (colz>=FMN_COLC) colz=FMN_COLC-1;
  if (cola>colz) return 0;
  int16_t rowa=y/FMN_MM_PER_TILE; if (rowa<0) rowa=0;
  int16_t rowz=(y+h-1)/FMN_MM_PER_TILE; if (rowz>=FMN_ROWC) rowz=FMN_ROWC-1;
  if (rowa>rowz) return 0;
  const uint8_t *cellrow=cellv+rowa*FMN_COLC+cola;
  for (;rowa<=rowz;rowa++,cellrow+=FMN_COLC) {
    const uint8_t *cellp=cellrow;
    int16_t col=cola;
    for (;col<=colz;col++,cellp++) {
      uint8_t physics=(propv[(*cellp)>>2]>>(6-(((*cellp)&3)<<1)))&3;
      if (features&(1<<physics)) return 1;
    }
  }
  return 0;
}

/* Sprites.
 */
 
static int fmn_sprite_collide_sprites(
  int16_t x,int16_t y,int16_t w,int16_t h,
  const struct fmn_sprite *source
) {
  int16_t r=x+w,b=y+h;
  struct fmn_sprite *sprite=fmn_spritev;
  uint8_t i=fmn_spritec;
  for (;i-->0;sprite++) {
    if (!sprite->type) continue;
    if (!(sprite->flags&FMN_SPRITE_FLAG_SOLID)) continue;
    if (sprite==source) continue;
    int16_t sx,sy,sw,sh;
    fmn_sprite_hitbox(&sx,&sy,&sw,&sh,sprite);
    if (sx>=r) continue;
    if (sy>=b) continue;
    if (sx+sw<=x) continue;
    if (sy+sh<=y) continue;
    return 1;
  }
  return 0;
}

/* Collide.
 */
 
int fmn_sprite_collide_box(int16_t x,int16_t y,int16_t w,int16_t h,struct fmn_sprite *source,uint8_t features) {
  
  if (features&FMN_SPRITE_COLLIDE_EDGES) {
    if (x<0) return 1;
    if (y<0) return 1;
    if (x+w>FMN_COLC*FMN_MM_PER_TILE) return 1;
    if (y+h>FMN_ROWC*FMN_MM_PER_TILE) return 1;
  }
  
  if (features&(
    FMN_SPRITE_COLLIDE_VACANT|
    FMN_SPRITE_COLLIDE_SOLID|
    FMN_SPRITE_COLLIDE_HOLE|
    FMN_SPRITE_COLLIDE_RESERVED
  )) {
    if (fmn_map.tilesheet&&fmn_map.tilesheet->tileprops) {
      if (fmn_sprite_collide_map(x,y,w,h,fmn_map.v,fmn_map.tilesheet->tileprops,features)) {
        return 1;
      }
    }
  }
  
  if (features&FMN_SPRITE_COLLIDE_SPRITES) {
    if (fmn_sprite_collide_sprites(x,y,w,h,source)) {
      return 1;
    }
  }
  return 0;
}
