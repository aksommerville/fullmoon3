#ifndef FMN_HERO_INTERNAL_H
#define FMN_HERO_INTERNAL_H

#include "fmn_hero.h"
#include "api/fmn_common.h"
#include "api/fmn_platform.h"
#include "game/play/fmn_play.h"
#include "game/image/fmn_image.h"
#include "game/map/fmn_map.h"
#include "game/sprite/fmn_sprite.h"
#include "game/fmn_data.h"
#include <string.h>
#include <stdio.h>

extern struct fmn_hero {
  int16_t x,y; // Center, mm.
  int8_t col,row; // Most recent position, in tiles.
  
  // Input state.
  int8_t indx,indy;
  uint8_t ina,inb;
  
  uint8_t facedir; // FMN_DIR_(N|S|W|E)
  uint8_t animclock;
  uint8_t animframe;
  int16_t walkspeed;
  int8_t walkdx,walkdy;
  
} fmn_hero;

// Call when input state changes. These functions will update (fmn_hero.in*)
void fmn_hero_event_dpad(int8_t x,int8_t y);
void fmn_hero_event_buttons(uint8_t a,uint8_t b);

void fmn_hero_update_walk();
void fmn_hero_update_animation();

/* Only the hero enjoys full collision detection (in other games, this would usually be a sprite concern).
 * I figure the hero will be the only one that needs the full treatment, and it's inconvenient to sync the sprite before and after.
 * Returns the absolute value of the final motion. Never returns negative.
 * X and Y axes are treated independently.
 * Motion should not be far enough to cross two cell boundaries, we assume there's no more than one.
 */
int16_t fmn_hero_move_with_physics(struct fmn_sprite **pumpkin,int16_t dx,int16_t dy);

#endif
