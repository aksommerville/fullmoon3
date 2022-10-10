/* fmn_sprite.h
 */
 
#ifndef FMN_SPRITE_H
#define FMN_SPRITE_H

#include <stdint.h>

struct fmn_sprite;
struct fmn_sprite_type;

/* Generic sprite instance.
 ********************************************************************/
 
#define FMN_SPRITE_BV_SIZE 8
#define FMN_SPRITE_SV_SIZE 4
 
struct fmn_sprite {
  const struct fmn_sprite_type *type; // null means sprite unused
  const struct fmn_image *tilesheet;
  int16_t x,y; // mm
  int8_t layer;
  uint8_t tileid,xform;
  uint8_t bv[FMN_SPRITE_BV_SIZE];
  int16_t sv[FMN_SPRITE_SV_SIZE];
};

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
};

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

#endif
