#include "fmn_hero_internal.h"
#include "game/fmn_game_mode.h"

/* Broom.
 */
 
static uint8_t fmn_hero_is_over_hole() {
  return fmn_hero_get_physics_underfoot()&(1<<2);
}
 
static void fmn_hero_update_broom() {
  if (!fmn_hero.ina) {
    // Released key but action is still active. We're waiting to get off the hole.
    if (!fmn_hero_is_over_hole()) {
      fmn_hero.action=-1;
    }
  }
}

static void fmn_hero_broom_end() {
  if (fmn_hero_is_over_hole()) {
    // Keep it up.
  } else {
    fmn_hero.action=-1;
  }
}

/* Feather.
 * We just look for a featherable sprite under it, and call that sprite.
 * Other sprites do all the interesting feather things.
 */
 
static void fmn_hero_update_feather() {
  int16_t x=fmn_hero.x,y=fmn_hero.y;
  uint8_t tdir;
  switch (fmn_hero.facedir) {
    case FMN_DIR_W: x-=FMN_MM_PER_TILE; tdir=FMN_DIR_E; break;
    case FMN_DIR_E: x+=FMN_MM_PER_TILE; tdir=FMN_DIR_W; break;
    case FMN_DIR_N: y-=FMN_MM_PER_TILE; tdir=FMN_DIR_S; break;
    case FMN_DIR_S: y+=FMN_MM_PER_TILE; tdir=FMN_DIR_N; break;
    default: return;
  }
  int16_t r=FMN_MM_PER_TILE>>1;
  int16_t nr=-r;
  uint8_t i=fmn_spritec;
  struct fmn_sprite *sprite=fmn_spritev;
  for (;i-->0;sprite++) {
    if (!sprite->type||!sprite->type->feather) continue;
    int16_t d=x-sprite->x;
    if (d>=r) continue;
    if (d<=nr) continue;
    d=y-sprite->y;
    if (d>=r) continue;
    if (d<=nr) continue;
    sprite->type->feather(sprite,tdir);
    return;
  }
}

/* Update action.
 */
 
void fmn_hero_update_action() {
  if (fmn_hero.action<0) return;
  fmn_hero.actiontime++;
  switch (fmn_hero.action) {
    case FMN_ITEM_broom: fmn_hero_update_broom(); break;
    case FMN_ITEM_feather: fmn_hero_update_feather(); break;
    case FMN_ITEM_wand: break;//TODO
    case FMN_ITEM_violin: break;//TODO
    case FMN_ITEM_match: break;//TODO
    case FMN_ITEM_umbrella: break;//TODO
    case FMN_ITEM_compass: break;//TODO
  }
}

/* Begin action.
 */
 
static void fmn_hero_begin_action() {
  if (fmn_hero.action>=0) return; // shouldn't happen
  uint8_t action=fmn_state_get_selected_item();
  if (!(fmn_state_get_possessed_items()&(1<<action))) return;
  fmn_hero.actiontime=0;
  switch (fmn_hero.action=action) {
    case FMN_ITEM_broom: break;
    case FMN_ITEM_feather: break;
    case FMN_ITEM_wand: break;//TODO
    case FMN_ITEM_violin: break;//TODO
    case FMN_ITEM_bell: break;//TODO
    case FMN_ITEM_chalk: break;//TODO
    case FMN_ITEM_pitcher: break;//TODO
    case FMN_ITEM_coin: break;//TODO
    case FMN_ITEM_match: break;//TODO
    case FMN_ITEM_corn: break;//TODO
    case FMN_ITEM_umbrella: break;//TODO
    case FMN_ITEM_shovel: break;//TODO
    case FMN_ITEM_compass: break;//TODO
    default: { // undefined action, drop it.
        fmn_hero.action=-1;
      }
  }
}

/* End action.
 */
 
static void fmn_hero_end_action() {
  switch (fmn_hero.action) {
    case FMN_ITEM_broom: fmn_hero_broom_end(); return;
    case FMN_ITEM_wand: break;//TODO commit spell
    case FMN_ITEM_violin: break;//TODO resume music
    case FMN_ITEM_match: break;//TODO turn lights back off
  }
  fmn_hero.action=-1;
}

/* Dpad changed.
 */
 
void fmn_hero_event_dpad(int8_t x,int8_t y) {
  fmn_hero.indx=x;
  fmn_hero.indy=y;
  if (x) fmn_hero.stickydx=x;
  if (y) fmn_hero.stickydy=y;
  
       if (x<0) fmn_hero.facedir=FMN_DIR_W;
  else if (x>0) fmn_hero.facedir=FMN_DIR_E;
  else if (y<0) fmn_hero.facedir=FMN_DIR_N;
  else if (y>0) fmn_hero.facedir=FMN_DIR_S;
}

/* Buttons changed.
 */
 
void fmn_hero_event_buttons(uint8_t a,uint8_t b) {
  if (a!=fmn_hero.ina) {
    if (fmn_hero.ina=a) fmn_hero_begin_action();
    else fmn_hero_end_action();
  }
  if (b!=fmn_hero.inb) {
    if (fmn_hero.inb=b) {
      fmn_hero_end_action();
      fmn_game_mode_set_pause();
    }
  }
}
