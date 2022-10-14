#include "game/sprite/fmn_sprite.h"
#include "game/fmn_data.h"
#include "game/image/fmn_image.h"

#define animframe sprite->bv[0]
#define animclock sprite->bv[1]
#define tileid0 sprite->bv[2]

/* Init.
 */
 
static void _bug_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->tilesheet=&fmnr_image_sprites1;
  sprite->tileid=0x00;
  sprite->xform=0;
  sprite->flags=0;
  tileid0=sprite->tileid;
}

/* Update.
 */
 
static void _bug_update(struct fmn_sprite *sprite) {
  if (animclock) {
    animclock--;
  } else {
    animclock=5;
    animframe++;
    if (animframe>=5) animframe=0;
    sprite->tileid=tileid0+animframe;
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_bug={
  .name="bug",
  .init=_bug_init,
  .update=_bug_update,
};
