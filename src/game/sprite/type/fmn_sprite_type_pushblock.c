#include "game/sprite/fmn_sprite.h"
#include "game/fmn_data.h"
#include "game/hero/fmn_hero.h"
#include <stdio.h>

#define FMN_PUSHBLOCK_MOVE_SPEED (FMN_MM_PER_TILE/16)
#define FMN_PUSHBLOCK_CHARM_SPEED (FMN_MM_PER_TILE/64)

#define movec sprite->bv[0]
#define charmdir sprite->bv[1]
#define movedx sprite->sv[0]
#define movedy sprite->sv[1]

/* Init.
 */
 
static void _pushblock_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->tilesheet=&fmnr_image_sprites1;
  sprite->tileid=0x0a;
  sprite->flags=FMN_SPRITE_FLAG_SOLID;
  sprite->layer=-1;
}

/* Move. Zero if blocked. We move only exactly what's asked for, or nothing.
 */
 
static uint8_t fmn_pushblock_move(struct fmn_sprite *sprite,int16_t dx,int16_t dy) {
  sprite->x+=dx;
  sprite->y+=dy;
  if (fmn_sprite_collide(sprite,FMN_SPRITE_COLLIDE_SOLID|FMN_SPRITE_COLLIDE_HOLE|FMN_SPRITE_COLLIDE_SPRITES)) {
    sprite->x-=dx;
    sprite->y-=dy;
    return 0;
  }
  return 1;
}

/* Update.
 */
 
static void _pushblock_update(struct fmn_sprite *sprite) {
  if (movec) {
    movec--;
    if (!fmn_pushblock_move(sprite,movedx*FMN_PUSHBLOCK_MOVE_SPEED,movedy*FMN_PUSHBLOCK_MOVE_SPEED)) {
      movec=0;
    }
  } else if (charmdir) {
    int16_t herox,heroy;
    fmn_hero_get_position(&herox,&heroy);
    int16_t r=FMN_MM_PER_TILE>>1;
    switch (charmdir) {
      case FMN_DIR_W: {
          if (heroy<sprite->y-r) return;
          if (heroy>sprite->y+r) return;
          if (herox>sprite->x-r) return;
          fmn_pushblock_move(sprite,-FMN_PUSHBLOCK_CHARM_SPEED,0);
        } break;
      case FMN_DIR_E: {
          if (heroy<sprite->y-r) return;
          if (heroy>sprite->y+r) return;
          if (herox<sprite->x+r) return;
          fmn_pushblock_move(sprite,FMN_PUSHBLOCK_CHARM_SPEED,0);
        } break;
      case FMN_DIR_N: {
          if (herox<sprite->x-r) return;
          if (herox>sprite->x+r) return;
          if (heroy>sprite->y-r) return;
          fmn_pushblock_move(sprite,0,-FMN_PUSHBLOCK_CHARM_SPEED);
        } break;
      case FMN_DIR_S: {
          if (herox<sprite->x-r) return;
          if (herox>sprite->x+r) return;
          if (heroy<sprite->y+r) return;
          fmn_pushblock_move(sprite,0,FMN_PUSHBLOCK_CHARM_SPEED);
        } break;
    }
  }
}

/* Push.
 */
 
static void _pushblock_push(struct fmn_sprite *sprite,int8_t dx,int8_t dy) {
  if (movec) return;
  if (dx&&dy) return;
  if (!dx&&!dy) return;
  movedx=dx;
  movedy=dy;
  movec=FMN_MM_PER_TILE/FMN_PUSHBLOCK_MOVE_SPEED;
}

/* Feather.
 */
 
static void _pushblock_feather(struct fmn_sprite *sprite,uint8_t dir) {
  charmdir=dir;
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_pushblock={
  .name="pushblock",
  .init=_pushblock_init,
  .update=_pushblock_update,
  .push=_pushblock_push,
  .feather=_pushblock_feather,
};
