#include "game/sprite/fmn_sprite.h"
#include "game/fmn_data.h"
#include "game/hero/fmn_hero.h"
#include "game/play/fmn_play.h"
#include <stdio.h>

#define FMN_ALPHABET_MOVE_SPEED (FMN_MM_PER_TILE/16)
#define FMN_ALPHABET_FLOAT_SPEED (FMN_MM_PER_TILE/64)

#define FMN_ALPHABET_FEATURES_SLIDE (FMN_SPRITE_COLLIDE_SOLID|FMN_SPRITE_COLLIDE_HOLE|FMN_SPRITE_COLLIDE_SPRITES)
#define FMN_ALPHABET_FEATURES_FLOAT (FMN_SPRITE_COLLIDE_SOLID|FMN_SPRITE_COLLIDE_SPRITES)

/* (spell) is FMN_DIR_(W,E,N,S) packed 2 bits each, big-endianly.
 * (spellc) is 0..4.
 */
#define movec sprite->bv[0]
#define spell sprite->bv[1]
#define spellc sprite->bv[2]
#define lastdir sprite->bv[3]
#define movedx sprite->sv[0]
#define movedy sprite->sv[1]
#define updseq sprite->sv[2]
#define lastdir_updseq sprite->sv[3]

static uint8_t fmn_dir_2bit(uint8_t dir) {
  switch (dir) {
    case FMN_DIR_W: return 0;
    case FMN_DIR_E: return 1;
    case FMN_DIR_N: return 2;
    case FMN_DIR_S: return 3;
  }
  return 0;
}

/* Init.
 */
 
static void _alphabet_init(struct fmn_sprite *sprite,const uint8_t *argv,uint8_t argc) {
  sprite->tilesheet=&fmnr_image_sprites1;
  sprite->tileid=0x0b;
  sprite->flags=FMN_SPRITE_FLAG_SOLID;
  sprite->layer=-1;
  if (argc>=1) sprite->tileid+=(argv[0]&3);
}

/* Move. Zero if blocked. We move only exactly what's asked for, or nothing.
 */
 
static uint8_t fmn_alphabet_move(struct fmn_sprite *sprite,int16_t dx,int16_t dy,uint8_t features) {
  sprite->x+=dx;
  sprite->y+=dy;
  if (fmn_sprite_collide(sprite,features)) {
    sprite->x-=dx;
    sprite->y-=dy;
    return 0;
  }
  return 1;
}

/* Check movement for lambda block.
 */
 
static void fmn_alphabet_check_lambda(struct fmn_sprite *sprite) {
  uint8_t facedir=fmn_hero_get_feather_dir();
  if (!facedir) return;
  int16_t r=FMN_MM_PER_TILE>>1;
  int16_t herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  switch (facedir) {
    case FMN_DIR_W: {
        if (heroy<sprite->y-r) return;
        if (heroy>sprite->y+r) return;
        if (herox<sprite->x) return;
        fmn_alphabet_move(sprite,FMN_ALPHABET_FLOAT_SPEED,0,FMN_ALPHABET_FEATURES_FLOAT);
      } break;
    case FMN_DIR_E: {
        if (heroy<sprite->y-r) return;
        if (heroy>sprite->y+r) return;
        if (herox>sprite->x) return;
        fmn_alphabet_move(sprite,-FMN_ALPHABET_FLOAT_SPEED,0,FMN_ALPHABET_FEATURES_FLOAT);
      } break;
    case FMN_DIR_N: {
        if (herox<sprite->x-r) return;
        if (herox>sprite->x+r) return;
        if (heroy<sprite->y) return;
        fmn_alphabet_move(sprite,0,FMN_ALPHABET_FLOAT_SPEED,FMN_ALPHABET_FEATURES_FLOAT);
      } break;
    case FMN_DIR_S: {
        if (herox<sprite->x-r) return;
        if (herox>sprite->x+r) return;
        if (heroy>sprite->y) return;
        fmn_alphabet_move(sprite,0,-FMN_ALPHABET_FLOAT_SPEED,FMN_ALPHABET_FEATURES_FLOAT);
      } break;
  }
}

/* Update.
 */
 
static void _alphabet_update(struct fmn_sprite *sprite) {
  updseq++;
  if (movec) {
    movec--;
    if (!fmn_alphabet_move(sprite,movedx*FMN_ALPHABET_MOVE_SPEED,movedy*FMN_ALPHABET_MOVE_SPEED,FMN_ALPHABET_FEATURES_SLIDE)) {
      movec=0;
    }
  } else if (sprite->tileid==0x0e) {
    fmn_alphabet_check_lambda(sprite);
  }
}

/* Spells.
 */
 
static void fmn_alphabet_check_spell_gamma(struct fmn_sprite *sprite) {
  if (spellc<3) return;
  if ((spell&0x3f)!=0x04) return; // WEW
  spellc=0;
  fmn_game_create_soulballs(sprite->x,sprite->y);
  sprite->type=0;
}
 
static void fmn_alphabet_check_spell_alpha(struct fmn_sprite *sprite) {
  if (spellc<4) return;
  int16_t dx=0,dy=0;
  switch (spell) {
    case 0x27: dy=-1; goto _move_; // WNES
    case 0x9c: dx= 1; goto _move_;// NESW
    case 0x72: dy= 1; goto _move_; // ESWN
    case 0xc9: dx=-1; goto _move_; // SWNE
    _move_: {
        spellc=0;
        movedx=dx;
        movedy=dy;
        movec=FMN_MM_PER_TILE/FMN_ALPHABET_MOVE_SPEED;
      }
  }
}
 
static void fmn_alphabet_check_spell_mu(struct fmn_sprite *sprite) {
  if (spellc<4) return;
  uint8_t a1=(spell>>6)&3;
  uint8_t a2=(spell>>4)&3;
  uint8_t b1=(spell>>2)&3;
  uint8_t a3=(spell   )&3;
  if ((a1!=a2)||(a1!=a3)||(a1==b1)) return; // AABA
  int16_t dx=0,dy=0;
  switch (a1) {
    case 0: dx=1; break;
    case 1: dx=-1; break;
    case 2: dy=1; break;
    case 3: dy=-1; break;
  }
  spellc=0;
  movedx=dx;
  movedy=dy;
  movec=FMN_MM_PER_TILE/FMN_ALPHABET_MOVE_SPEED;
}

/* Feather.
 */
 
static void _alphabet_feather(struct fmn_sprite *sprite,uint8_t dir) {
  
  dir=fmn_dir_2bit(dir);
  if (dir==lastdir) {
    if (updseq==lastdir_updseq+1) {
      lastdir_updseq=updseq;
      return;
    }
  }
  lastdir=dir;
  lastdir_updseq=updseq;
  spell<<=2;
  spell|=dir;
  if (spellc<4) spellc++;
  
  switch (sprite->tileid) {
    case 0x0b: fmn_alphabet_check_spell_gamma(sprite); break;
    case 0x0c: fmn_alphabet_check_spell_alpha(sprite); break;
    case 0x0d: fmn_alphabet_check_spell_mu(sprite); break;
    case 0x0e: break; // lambda works differently (see _alphabet_update)
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_type fmn_sprite_type_alphabet={
  .name="alphabet",
  .init=_alphabet_init,
  .update=_alphabet_update,
  .feather=_alphabet_feather,
};
