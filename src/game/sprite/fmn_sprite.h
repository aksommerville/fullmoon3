/* fmn_sprite.h
 */
 
#ifndef FMN_SPRITE_H
#define FMN_SPRITE_H

#include <stdint.h>
#include "api/fmn_common.h"

struct fmn_sprite;
struct fmn_sprite_type;
struct fmn_image;

/* Sprite type.
 *****************************************************************/
 
struct fmn_sprite_type {
  const char *name;
  void (*init)(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc);
  void (*update)(struct fmn_sprite *sprite);
  
  /* Ignore (sprite->x,y) and use the provided position instead, it's in pixels.
   * If not implemented, there is a generic fallback.
   */
  void (*render)(struct fmn_image *fb,struct fmn_sprite *sprite,int16_t x,int16_t y);
  
  /* Populate hitbox in mm.
   * If unset, the default is a FMN_MM_PER_TILE square centered on (sprite->x,y).
   */
  void (*hitbox)(int16_t *x,int16_t *y,int16_t *w,int16_t *h,struct fmn_sprite *sprite);
  
  // Hero walks directly into sprite (implies that sprite is SOLID).
  void (*push)(struct fmn_sprite *sprite,int8_t dx,int8_t dy);
  
  // Hero tickles this sprite with a feather, on this sprite's (dir) edge.
  // Will be called once per frame while the tickle continues.
  void (*feather)(struct fmn_sprite *sprite,uint8_t dir);
};

/* Generic sprite instance.
 ********************************************************************/
 
#define FMN_SPRITE_BV_SIZE 8
#define FMN_SPRITE_SV_SIZE 4

#define FMN_SPRITE_FLAG_SOLID 0x01
 
struct fmn_sprite {
  const struct fmn_sprite_type *type; // null means sprite unused
  const struct fmn_image *tilesheet;
  int16_t x,y; // mm
  int8_t layer;
  uint8_t tileid,xform;
  uint8_t flags;
  uint8_t bv[FMN_SPRITE_BV_SIZE];
  int16_t sv[FMN_SPRITE_SV_SIZE];
};

static inline void fmn_sprite_hitbox(int16_t *x,int16_t *y,int16_t *w,int16_t *h,struct fmn_sprite *sprite) {
  if (sprite->type&&sprite->type->hitbox) sprite->type->hitbox(x,y,w,h,sprite);
  else {
    *x=sprite->x-(FMN_MM_PER_TILE>>1);
    *y=sprite->y-(FMN_MM_PER_TILE>>1);
    *w=FMN_MM_PER_TILE;
    *h=FMN_MM_PER_TILE;
  }
}

/* Nonzero if a collision exists.
 * We don't correct or provide details.
 * Colliding against a box takes optional (source), one sprite to exclude from examination.
 */
int fmn_sprite_collide_box(int16_t x,int16_t y,int16_t w,int16_t h,struct fmn_sprite *source,uint8_t features);
static inline int fmn_sprite_collide(struct fmn_sprite *sprite,uint8_t features) {
  int16_t x,y,w,h;
  fmn_sprite_hitbox(&x,&y,&w,&h,sprite);
  return fmn_sprite_collide_box(x,y,w,h,sprite,features);
}
#define FMN_SPRITE_COLLIDE_VACANT    0x01 /* The low four bits correspond to the four map cell values. */
#define FMN_SPRITE_COLLIDE_SOLID     0x02 /* That's not a coincidence; we depend on it. */
#define FMN_SPRITE_COLLIDE_HOLE      0x04
#define FMN_SPRITE_COLLIDE_RESERVED  0x08
#define FMN_SPRITE_COLLIDE_EDGES     0x10
#define FMN_SPRITE_COLLIDE_SPRITES   0x20

/* There is a global list of active sprites.
 * It may be sparse, and objects may move around between updates.
 * Sprite addresses are stable except for the functions marked below.
 *******************************************************************/
 
#define FMN_SPRITE_LIMIT 64
 
extern struct fmn_sprite fmn_spritev[FMN_SPRITE_LIMIT];
extern uint8_t fmn_spritec;

/* These may move sprites around and change their addresses.
 * sort_partial() pushes holes to the back and removes them when they arrive.
 * sort_full() removes all holes.
 */
void fmn_spritev_sort_partial();
void fmn_spritev_sort_full();

/* You must set sprite->type after allocation, otherwise it is treated as unused.
 * We don't actually allocate any memory, this just finds a vacancy in the static list.
 */
struct fmn_sprite *fmn_sprite_alloc();

void fmn_spritev_clear();

void fmn_spritev_update();

/* Sprite types.
 ***********************************************************************/
 
extern const struct fmn_sprite_type fmn_sprite_type_dummy;
extern const struct fmn_sprite_type fmn_sprite_type_hero;
extern const struct fmn_sprite_type fmn_sprite_type_bug;
extern const struct fmn_sprite_type fmn_sprite_type_pushblock;

#endif
