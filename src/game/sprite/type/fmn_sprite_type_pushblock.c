#include "game/sprite/fmn_sprite.h"
#include "game/fmn_data.h"
#include <stdio.h>

#define FMN_PUSHBLOCK_MOVE_SPEED (FMN_MM_PER_TILE/16)

#define movec sprite->bv[0]
#define movedx sprite->sv[0]
#define movedy sprite->sv[1]

/* Init.
 */
 
static void _pushblock_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->tilesheet=&fmnr_image_sprites1;
  sprite->tileid=0x0a;
  sprite->flags=FMN_SPRITE_FLAG_SOLID;
}

/* Update.
 */
 
static void _pushblock_update(struct fmn_sprite *sprite) {
  if (movec) {
    movec--;
    sprite->x+=movedx*FMN_PUSHBLOCK_MOVE_SPEED;
    sprite->y+=movedy*FMN_PUSHBLOCK_MOVE_SPEED;
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
  )) return;
  movedx=dx;
  movedy=dy;
  movec=FMN_MM_PER_TILE/FMN_PUSHBLOCK_MOVE_SPEED;
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_pushblock={
  .name="pushblock",
  .init=_pushblock_init,
  .update=_pushblock_update,
  .push=_pushblock_push,
};
