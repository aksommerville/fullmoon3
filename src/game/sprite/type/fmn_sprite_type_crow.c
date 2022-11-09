#include "game/sprite/fmn_sprite.h"
#include "game/play/fmn_play.h"
#include "game/fmn_data.h"
#include "game/image/fmn_image.h"

#define FMN_CROW_HORZ_SPEED (FMN_MM_PER_TILE/15)
#define FMN_CROW_VERT_SPEED (FMN_MM_PER_TILE/19)

#define animclock sprite->bv[0]
#define animframe sprite->bv[1]
#define tileid0 sprite->bv[2]
#define landed sprite->bv[3]
// Keep (sv[0],sv[1]) as (targetx,targety); corn depends on it.
#define targetx sprite->sv[0]
#define targety sprite->sv[1]

/* Init.
 */
 
static void _crow_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->tilesheet=&fmnr_image_sprites1;
  sprite->tileid=0x1c;
  sprite->layer=20;
  tileid0=sprite->tileid;
}

/* Update.
 */

static void _crow_update(struct fmn_sprite *sprite) {

  if (animclock) {
    animclock--;
  } else if (landed) {
    animclock=10;
    if (++(animframe)>=2) animframe=0;
    sprite->tileid=tileid0+animframe*2;
  } else {
    animclock=5;
    if (++(animframe)>=2) animframe=0;
    sprite->tileid=tileid0+animframe;
  }
  
  if (landed) {
    // ok eating the corn, now what? TODO
    
  } else {
    //TODO Can we use a more refined line walker, draw a straight line right to the corn?
    if (sprite->x<targetx) {
      sprite->xform=0;
      sprite->x+=FMN_CROW_HORZ_SPEED;
      if (sprite->x>targetx) sprite->x=targetx;
    } else if (sprite->x>targetx) {
      sprite->xform=FMN_XFORM_XREV;
      sprite->x-=FMN_CROW_HORZ_SPEED;
      if (sprite->x<targetx) sprite->x=targetx;
    }
    if (sprite->y<targety) {
      sprite->y+=FMN_CROW_VERT_SPEED;
      if (sprite->y>targety) sprite->y=targety;
    } else if (sprite->y>targety) {
      sprite->y-=FMN_CROW_VERT_SPEED;
      if (sprite->y<targety) sprite->y=targety;
    }
    if ((sprite->x==targetx)&&(sprite->y==targety)) {
      landed=1;
      sprite->layer=0;
    }
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_crow={
  .name="crow",
  .init=_crow_init,
  .update=_crow_update,
};
