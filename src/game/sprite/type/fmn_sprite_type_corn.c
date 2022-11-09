#include "game/sprite/fmn_sprite.h"
#include "game/play/fmn_play.h"
#include "game/fmn_data.h"

#define FMN_CORN_CROW_TIME 300

#define animclock sprite->bv[0]
#define animframe sprite->bv[1]
#define tileid0 sprite->bv[2]
#define age sprite->sv[0]

/* Init.
 */
 
static void _corn_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->tilesheet=&fmnr_image_sprites1;
  sprite->tileid=0x19;
  sprite->layer=-1;
  tileid0=sprite->tileid;
}

/* Update.
 */

static void _corn_update(struct fmn_sprite *sprite) {

  if (animclock) {
    animclock--;
  } else {
    animclock=7;
    switch (animframe++) {
      case 0: sprite->tileid=tileid0+0; break;
      case 1: sprite->tileid=tileid0+1; break;
      case 2: sprite->tileid=tileid0+2; break;
      default: animframe=2;
    }
  }
  
  if (age<0x7fff) age++;
  if (age==FMN_CORN_CROW_TIME) {
    struct fmn_sprite *crow=fmn_game_spawn_sprite(&fmn_sprite_type_crow,0,0,0,0);
    if (crow) {
      crow->sv[0]=sprite->x;
      crow->sv[1]=sprite->y;
      if (sprite->x<(FMN_COLC*FMN_MM_PER_TILE)>>1) {
        crow->x=(FMN_COLC+1)*FMN_MM_PER_TILE;
        crow->sv[0]+=FMN_MM_PER_TILE>>1;
      } else {
        crow->x=-FMN_MM_PER_TILE;
        crow->sv[0]-=FMN_MM_PER_TILE>>1;
      }
    }
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_corn={
  .name="corn",
  .init=_corn_init,
  .update=_corn_update,
};
