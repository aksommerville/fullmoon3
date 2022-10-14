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
  if ((dx||dy)&&!(dx&&dy)) {
    //TODO Check map and solid sprites, confirm the space is fully open.
    movedx=dx;
    movedy=dy;
    movec=FMN_MM_PER_TILE/FMN_PUSHBLOCK_MOVE_SPEED;
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_pushblock={
  .name="pushblock",
  .init=_pushblock_init,
  .update=_pushblock_update,
  .push=_pushblock_push,
};
