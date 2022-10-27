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

/* Update.
 */
 
static void _pushblock_update(struct fmn_sprite *sprite) {
  if (movec) {
    movec--;
    sprite->x+=movedx*FMN_PUSHBLOCK_MOVE_SPEED;
    sprite->y+=movedy*FMN_PUSHBLOCK_MOVE_SPEED;
  } else if (charmdir) {
    int16_t herox,heroy;
    fmn_hero_get_position(&herox,&heroy);
    int16_t r=FMN_MM_PER_TILE>>1;
    switch (charmdir) {
      case FMN_DIR_W: {
          if (heroy<sprite->y-r) return;
          if (heroy>sprite->y+r) return;
          if (herox>sprite->x-r) return;
          sprite->x-=FMN_PUSHBLOCK_CHARM_SPEED;
          if (fmn_sprite_collide(sprite,FMN_SPRITE_COLLIDE_SOLID|FMN_SPRITE_COLLIDE_HOLE|FMN_SPRITE_COLLIDE_SPRITES)) {
            sprite->x+=FMN_PUSHBLOCK_CHARM_SPEED;
          }
        } break;
      case FMN_DIR_E: {
          if (heroy<sprite->y-r) return;
          if (heroy>sprite->y+r) return;
          if (herox<sprite->x+r) return;
          sprite->x+=FMN_PUSHBLOCK_CHARM_SPEED;
          if (fmn_sprite_collide(sprite,FMN_SPRITE_COLLIDE_SOLID|FMN_SPRITE_COLLIDE_HOLE|FMN_SPRITE_COLLIDE_SPRITES)) {
            sprite->x-=FMN_PUSHBLOCK_CHARM_SPEED;
          }
        } break;
      case FMN_DIR_N: {
          if (herox<sprite->x-r) return;
          if (herox>sprite->x+r) return;
          if (heroy>sprite->y-r) return;
          sprite->y-=FMN_PUSHBLOCK_CHARM_SPEED;
          if (fmn_sprite_collide(sprite,FMN_SPRITE_COLLIDE_SOLID|FMN_SPRITE_COLLIDE_HOLE|FMN_SPRITE_COLLIDE_SPRITES)) {
            sprite->y+=FMN_PUSHBLOCK_CHARM_SPEED;
          }
        } break;
      case FMN_DIR_S: {
          if (herox<sprite->x-r) return;
          if (herox>sprite->x+r) return;
          if (heroy<sprite->y+r) return;
          sprite->y+=FMN_PUSHBLOCK_CHARM_SPEED;
          if (fmn_sprite_collide(sprite,FMN_SPRITE_COLLIDE_SOLID|FMN_SPRITE_COLLIDE_HOLE|FMN_SPRITE_COLLIDE_SPRITES)) {
            sprite->y-=FMN_PUSHBLOCK_CHARM_SPEED;
          }
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
  int16_t testx,testy,testw,testh;
  fmn_sprite_hitbox(&testx,&testy,&testw,&testh,sprite);
  testx+=8; testy+=8; testw-=16; testh-=16;
  if (dx<0) { testx-=testw; testw<<=1; }
  else if (dx>0) testw<<=1;
  else if (dy<0) { testy-=testh; testh<<=1; }
  else testh<<=1;
  if (fmn_sprite_collide_box(
    testx,testy,testw,testh,sprite,
    FMN_SPRITE_COLLIDE_SOLID|
    FMN_SPRITE_COLLIDE_HOLE|
    FMN_SPRITE_COLLIDE_EDGES|
    FMN_SPRITE_COLLIDE_SPRITES
  )) {
    return;
  }
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
