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
  
} fmn_hero;

// Call when input state changes. These functions will update (fmn_hero.in*)
void fmn_hero_event_dpad(int8_t x,int8_t y);
void fmn_hero_event_buttons(uint8_t a,uint8_t b);

void fmn_hero_update_walk();
void fmn_hero_update_animation();

#endif
