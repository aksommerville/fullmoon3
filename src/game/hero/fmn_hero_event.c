#include "fmn_hero_internal.h"
#include "game/fmn_game_mode.h"

/* Broom.
 */
 
static uint8_t fmn_hero_is_over_hole() {
  return fmn_hero_get_physics_underfoot()&(1<<2);
}
 
static void fmn_hero_broom_update() {
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
 
static void fmn_hero_feather_update() {
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

/* Wand.
 */
 
static void fmn_hero_wand_begin() {
  fmn_hero.walkdx=0;
  fmn_hero.walkdy=0;
  fmn_hero.wanddir=0;
  fmn_hero.spellc=0;
}

static void fmn_hero_wand_update() {

  uint8_t wanddir=0;
  if (fmn_hero.indx&&fmn_hero.indy) ; // conflict, call it none
  else if (fmn_hero.indx<0) wanddir=FMN_DIR_W;
  else if (fmn_hero.indx>0) wanddir=FMN_DIR_E;
  else if (fmn_hero.indy<0) wanddir=FMN_DIR_N;
  else if (fmn_hero.indy>0) wanddir=FMN_DIR_S;
  
  if (wanddir!=fmn_hero.wanddir) {
    fmn_hero.wanddir=wanddir;
    if (wanddir) {
      if (fmn_hero.spellc<FMN_SPELL_LIMIT) {
        fmn_hero.spellv[fmn_hero.spellc++]=wanddir;
      }
    }
  }
}

static void fmn_hero_wand_end() {

  uint8_t ok=1;
  if (fmn_hero.spellc) {
    // If it's the length of our buffer, don't even send it. It could have been clipped.
    if (fmn_hero.spellc<FMN_SPELL_LIMIT) {
      ok=fmn_game_cast_spell(fmn_hero.spellv,fmn_hero.spellc);
    } else {
      ok=0;
    }
    fmn_hero.spellc=0;
  }

  if (!ok) {
    fprintf(stderr,"TODO repudiate invalid spell\n");//TODO Repudiation if unknown and not empty.
  }
  
  // We always face south during encode. I think it's agreeable to stay south after.
  fmn_hero.facedir=FMN_DIR_S;
  // Force reconsideration of dpad (ensure if we start walking, we face the right direction).
  fmn_hero.indx=0;
  fmn_hero.indy=0;
}

/* Violin.
 */
 
static void fmn_hero_violin_begin() {
  fmn_hero.walkdx=0;
  fmn_hero.walkdy=0;
  fmn_hero.violindir=0;
  fmn_violin_begin();
}

static void fmn_hero_violin_update() {

  if (fmn_hero.indx&&fmn_hero.indy) {
    fmn_violin_update(fmn_hero.violindir);
    return;
  }

  // Was going to do diagonals but that's really hard to manage finger-wise.
  uint8_t violindir=0;
       if (fmn_hero.indx<0) violindir=FMN_DIR_W;
  else if (fmn_hero.indx>0) violindir=FMN_DIR_E;
  else if (fmn_hero.indy<0) violindir=FMN_DIR_N;
  else if (fmn_hero.indy>0) violindir=FMN_DIR_S;
  
  fmn_violin_update(violindir);
  fmn_hero.violindir=violindir;
}

static void fmn_hero_violin_end() {
  fmn_violin_end();
  
  // We always face south during encode. I think it's agreeable to stay south after.
  fmn_hero.facedir=FMN_DIR_S;
  // Force reconsideration of dpad (ensure if we start walking, we face the right direction).
  fmn_hero.indx=0;
  fmn_hero.indy=0;
}

/* Bell.
 */
 
static void fmn_hero_bell_update() {
  if (!(fmn_hero.actiontime%20)) {
    fmn_platform_audio_note(0,0x60,0x70,100);
  }
}

static void fmn_hero_bell_begin() {
  fmn_hero_bell_update();
  //TODO fire event
}

/* Update action.
 */
 
void fmn_hero_update_action() {
  if (fmn_hero.action<0) return;
  fmn_hero.actiontime++;
  switch (fmn_hero.action) {
    case FMN_ITEM_broom: fmn_hero_broom_update(); break;
    case FMN_ITEM_feather: fmn_hero_feather_update(); break;
    case FMN_ITEM_wand: fmn_hero_wand_update(); break;
    case FMN_ITEM_violin: fmn_hero_violin_update(); break;
    case FMN_ITEM_bell: fmn_hero_bell_update(); break;
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
    case FMN_ITEM_wand: fmn_hero_wand_begin(); break;
    case FMN_ITEM_violin: fmn_hero_violin_begin(); break;
    case FMN_ITEM_bell: fmn_hero_bell_begin(); break;
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
    case FMN_ITEM_broom: fmn_hero_broom_end(); return; // sic return; might retain (action)
    case FMN_ITEM_wand: fmn_hero_wand_end(); break;
    case FMN_ITEM_violin: fmn_hero_violin_end(); break;
    case FMN_ITEM_match: break;//TODO turn lights back off
  }
  fmn_hero.action=-1;
}

/* Dpad changed.
 */
 
static uint8_t fmn_hero_can_change_direction() {
  if (fmn_hero.action==FMN_ITEM_wand) return 0;
  return 1;
}
 
void fmn_hero_event_dpad(int8_t x,int8_t y) {
  fmn_hero.indx=x;
  fmn_hero.indy=y;
  if (x) fmn_hero.stickydx=x;
  if (y) fmn_hero.stickydy=y;
  
  if (fmn_hero_can_change_direction()) {
         if (x<0) fmn_hero.facedir=FMN_DIR_W;
    else if (x>0) fmn_hero.facedir=FMN_DIR_E;
    else if (y<0) fmn_hero.facedir=FMN_DIR_N;
    else if (y>0) fmn_hero.facedir=FMN_DIR_S;
  }
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
