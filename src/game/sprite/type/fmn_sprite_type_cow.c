#include "game/sprite/fmn_sprite.h"
#include "game/fmn_data.h"
#include "game/image/fmn_image.h"

#define FMN_COW_MUNCH_FRAMEC 20
#define FMN_COW_GRASS_TIME 40

#define animframe sprite->bv[0]
#define animclock sprite->bv[1]
#define tileid0 sprite->bv[2]

/* Init.
 */
 
static void _cow_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->tilesheet=&fmnr_image_sprites1;
  sprite->tileid=0x16;
  sprite->xform=0;
  sprite->flags=FMN_SPRITE_FLAG_SOLID;
  tileid0=sprite->tileid;
}

/* Update.
 */
 
static void _cow_update(struct fmn_sprite *sprite) {
  if (animclock) {
    animclock--;
  } else {
    animframe++;
    animclock=15;
    if (animframe<FMN_COW_MUNCH_FRAMEC) {
      sprite->tileid=tileid0+(animframe&1);
    } else if (animframe==FMN_COW_MUNCH_FRAMEC) {
      sprite->tileid=tileid0+2;
      animclock=FMN_COW_GRASS_TIME;
    } else {
      animframe=0;
      sprite->tileid=tileid0;
    }
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_cow={
  .name="cow",
  .init=_cow_init,
  .update=_cow_update,
};
