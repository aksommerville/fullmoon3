/* fmn_hero.h
 * The hero is a sprite, but also a global singleton, because it does way more than most sprites.
 */
 
#ifndef FMN_HERO_H
#define FMN_HERO_H

#include <stdint.h>

void fmn_hero_reset();

/* set_position quietly updates our cell memory, so cell-related actions will *not* trigger.
 */
void fmn_hero_get_position(int16_t *xmm,int16_t *ymm);
void fmn_hero_set_position(int16_t xmm,int16_t ymm);

void fmn_hero_input(uint8_t input,uint8_t pvinput);
void fmn_hero_update();

// If hero is currently wagging the feather, FMN_DIR_(W,E,N,S) the direction she's facing.
uint8_t fmn_hero_get_feather_dir();

#endif
