#include "game/sprite/fmn_sprite.h"
#include "game/fmn_data.h"
#include "game/image/fmn_image.h"

/* Init.
 */
 
static void _beehive_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->tilesheet=&fmnr_image_sprites1;
  sprite->tileid=0x0f;
  sprite->xform=0;
  sprite->flags=0;
}

//TODO Animate? I'm thinking wobble up and down a little.
// Maybe some like 1-pixel bees flying around?

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_beehive={
  .name="beehive",
  .init=_beehive_init,
};
