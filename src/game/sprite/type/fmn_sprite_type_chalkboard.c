/* fmn_sprite_type_chalkboard.c
 * Chalkboard is a placeholder sprite that shows the hero she is able to draw here, and renders that drawing.
 * It doesn't have any behavior, or any appearance beyond the user-supplied chalk lines.
 */

#include "game/sprite/fmn_sprite.h"
#include "game/image/fmn_image.h"

/* A 7x7 image telling for each pixel of output, which bit (0..19) of the source image does it come from.
 * Any cell with an OOB value is set if any of its neighbors is.
 * The source image is stored little-endianly in bv[0,1,2].
 */
static const uint8_t fmn_chalkboard_bits[49]={
  99,19,19,99,18,18,99,
  17,16,15,14,13,12,11,
  17,15,16,14,12,13,11,
  99,10,10,99, 9, 9,99,
   8, 7, 6, 5, 4, 3, 2,
   8, 6, 7, 5, 3, 4, 2,
  99, 1, 1,99, 0, 0,99,
};

/* Init.
 */
 
static void _chalkboard_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->flags=FMN_SPRITE_FLAG_SOLID;
  sprite->layer=-100;
}

/* Render.
 */
 
static void _chalkboard_render(struct fmn_image *dst,struct fmn_sprite *sprite,int16_t x,int16_t y) {
  x-=FMN_TILESIZE>>1;
  y-=FMN_TILESIZE>>1;
  // We must be aligned with the grid and can't move.
  // Therefore we can't ever be partially offscreen.
  // If that ever happens, just get out.
  if ((x<0)||(x+7>dst->w)) return;
  if ((y<0)||(y+7>dst->h)) return;
  
  uint32_t srcimage=sprite->bv[0]|(sprite->bv[1]<<8)|(sprite->bv[2]<<16);
  if (srcimage) {
    const uint8_t *bitassign=fmn_chalkboard_bits;
    int16_t suby=0; for (;suby<7;suby++) {
      int16_t subx=0; for (;subx<7;subx++,bitassign++) {
        if (*bitassign>=20) {
          // Check neighbors. Vertices are never adjacent, so at least we have that going for us.
          if (
            ((subx>0)&&(srcimage&(1<<bitassign[-1])))||
            ((subx<6)&&(srcimage&(1<<bitassign[1])))||
            ((suby>0)&&(srcimage&(1<<bitassign[-7])))||
            ((suby<6)&&(srcimage&(1<<bitassign[7])))||
            ((subx>0)&&(suby>0)&&(srcimage&(1<<bitassign[-8])))||
            ((subx>0)&&(suby<6)&&(srcimage&(1<<bitassign[6])))||
            ((subx<6)&&(suby>0)&&(srcimage&(1<<bitassign[-6])))||
            ((subx<6)&&(suby<6)&&(srcimage&(1<<bitassign[8])))||
          0) {
            fmn_image_fill_rect(dst,x+subx,y+suby,1,1,3);
          }
        } else if (srcimage&(1<<*bitassign)) {
          fmn_image_fill_rect(dst,x+subx,y+suby,1,1,3);
        }
      }
    }
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_chalkboard={
  .name="chalkboard",
  .init=_chalkboard_init,
  .render=_chalkboard_render,
};
