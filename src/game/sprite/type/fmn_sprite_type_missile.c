#include "game/sprite/fmn_sprite.h"
#include "game/fmn_data.h"
#include <math.h>

#define dx sprite->sv[0]
#define dy sprite->sv[1]

/* Init.
 */
 
static void _missile_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->tilesheet=&fmnr_image_sprites1;
}

void fmn_sprite_missile_setup(struct fmn_sprite *sprite,int16_t dxmm,int16_t dymm,int16_t speedmm) {
  float dx2=(float)dxmm*(float)dxmm;
  float dy2=(float)dymm*(float)dymm;
  int16_t distance=sqrtf(dx2+dy2);
  if (distance) {
    dx=(dxmm*speedmm)/distance;
    dy=(dymm*speedmm)/distance;
  }
  if (!dx&&!dy) {
    sprite->type=0;
  }
}

/* Update.
 */
 
static void _missile_update(struct fmn_sprite *sprite) {
  sprite->x+=dx;
  sprite->y+=dy;
  if (
    (sprite->x<-FMN_MM_PER_TILE)||
    (sprite->y<-FMN_MM_PER_TILE)||
    (sprite->x>(FMN_COLC+1)*FMN_MM_PER_TILE)||
    (sprite->y>(FMN_ROWC+1)*FMN_MM_PER_TILE)
  ) {
    sprite->type=0;
  }
  //TODO collisions etc... what do missiles do?
  //TODO animation?
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_missile={
  .name="missile",
  .init=_missile_init,
  .update=_missile_update,
};
