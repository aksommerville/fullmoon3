#include "fmn_hero_internal.h"
#include "game/fmn_game_mode.h"

/* Update action.
 */
 
void fmn_hero_update_action() {
  switch (fmn_hero.action) {
    case FMN_ITEM_feather: break;//TODO
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
  fprintf(stderr,"TODO %s %d\n",__func__,action);
  switch (fmn_hero.action=action) {
    case FMN_ITEM_broom: break;//TODO
    case FMN_ITEM_feather: break;//TODO
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
  fprintf(stderr,"TODO %s\n",__func__);
  switch (fmn_hero.action) {
#define FMN_ITEM_broom       0
#define FMN_ITEM_feather     1
#define FMN_ITEM_wand        2
#define FMN_ITEM_violin      3
#define FMN_ITEM_bell        4
#define FMN_ITEM_chalk       5
#define FMN_ITEM_pitcher     6 /* "count" is actually an enum for the pitcher's contents */
#define FMN_ITEM_coin        7 /* counted */
#define FMN_ITEM_match       8 /* counted */
#define FMN_ITEM_corn        9 /* counted */
#define FMN_ITEM_umbrella   10
#define FMN_ITEM_shovel     11
#define FMN_ITEM_compass    12
    case FMN_ITEM_broom: break;//TODO prevent end if over a hole
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
