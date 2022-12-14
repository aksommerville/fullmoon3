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

/* Chalk.
 */
 
static struct fmn_sprite *fmn_hero_find_chalkboard() {
  if (fmn_hero.facedir!=FMN_DIR_N) return 0;
  int16_t x=fmn_hero.x;
  int16_t y=fmn_hero.y-FMN_MM_PER_TILE;
  int16_t r=FMN_MM_PER_TILE>>1;
  struct fmn_sprite *sprite=fmn_spritev;
  uint8_t i=fmn_spritec;
  for (;i-->0;sprite++) {
    if (sprite->type!=&fmn_sprite_type_chalkboard) continue;
    if (x<sprite->x-r) continue;
    if (x>sprite->x+r) continue;
    if (y<sprite->y-r) continue;
    if (y>sprite->y+r) continue;
    return sprite;
  }
  return 0;
}
 
static void fmn_hero_chalk_begin() {
  // Chalk is only meaningful when facing north, into a chalkboard sprite.
  // When that happens, we switch out to a completely different app mode.
  struct fmn_sprite *chalkboard=fmn_hero_find_chalkboard();
  if (chalkboard) {
    fmn_game_mode_set_chalk(chalkboard);
  }
  fmn_hero.action=-1;
}

/* Pitcher.
 */
 
static uint8_t fmn_hero_find_pitcher_content(int16_t x,int16_t y) {
  
  // Look for milkable sprites.
  const int16_t r=FMN_MM_PER_TILE>>1;
  int16_t left=x-r,right=x+r,top=y-r,bottom=y+r;
  struct fmn_sprite *cow=fmn_spritev;
  uint8_t i=fmn_spritec;
  for (;i-->0;cow++) {
    if (cow->x<left) continue;
    if (cow->x>right) continue;
    if (cow->y<top) continue;
    if (cow->y>bottom) continue;
    if (cow->type==&fmn_sprite_type_beehive) return FMN_PITCHER_CONTENT_honey;
    if (cow->type==&fmn_sprite_type_cow) return FMN_PITCHER_CONTENT_milk;
  }
  
  // Look for map cells.
  // We don't have anywhere to record per-tile which ones are milkable.
  // Hard-coding them here. Probably a bad idea.
  if ((x>=0)&&(y>=0)&&(x<FMN_COLC*FMN_MM_PER_TILE)&&(y<FMN_ROWC*FMN_MM_PER_TILE)) {
    uint8_t col=x/FMN_MM_PER_TILE;
    uint8_t row=y/FMN_MM_PER_TILE;
    uint8_t tileid=fmn_map.v[row*FMN_COLC+col];
    if (fmn_map.tilesheet==&fmnr_image_outdoors) {
      if ((tileid>=0x15)&&(tileid<=0x19)) return FMN_PITCHER_CONTENT_water;
      if ((tileid>=0x25)&&(tileid<=0x29)) return FMN_PITCHER_CONTENT_water;
      if ((tileid>=0x35)&&(tileid<=0x39)) return FMN_PITCHER_CONTENT_water;
      if ((tileid>=0x1a)&&(tileid<=0x1e)) return FMN_PITCHER_CONTENT_sap;
      if ((tileid>=0x2a)&&(tileid<=0x2e)) return FMN_PITCHER_CONTENT_sap;
      if ((tileid>=0x3a)&&(tileid<=0x3e)) return FMN_PITCHER_CONTENT_sap;
      if ((tileid>=0x89)&&(tileid<=0x8b)) return FMN_PITCHER_CONTENT_sap;
      if (tileid==0x99) return FMN_PITCHER_CONTENT_sap;
      if (tileid==0x9b) return FMN_PITCHER_CONTENT_sap;
    }
  }
  
  return FMN_PITCHER_CONTENT_none;
}

static void fmn_hero_log_pitcher(const char *verb,uint8_t content) { //XXX
  const char *contentname="UNKNOWN";
  switch (content) {
    #define _(tag) case FMN_PITCHER_CONTENT_##tag: contentname=#tag; break;
    FMN_FOR_EACH_PITCHER_CONTENT
    #undef _
  }
  fprintf(stderr,"Pitcher: %s %s\n",verb,contentname);
}
 
static void fmn_hero_pitcher_begin() {
  int16_t x=fmn_hero.x,y=fmn_hero.y;
  switch (fmn_hero.facedir) {
    case FMN_DIR_N: y-=FMN_MM_PER_TILE; break;
    case FMN_DIR_S: y+=FMN_MM_PER_TILE; break;
    case FMN_DIR_W: x-=FMN_MM_PER_TILE; break;
    case FMN_DIR_E: x+=FMN_MM_PER_TILE; break;
  }
  uint8_t content=fmn_state_get_item_count(FMN_ITEM_pitcher);
  if (content) {
    fmn_hero_log_pitcher("pour",content);
    fmn_state_set_item_count(FMN_ITEM_pitcher,0);
    fmn_game_pour_fluid(x,y,content);
  } else {
    content=fmn_hero_find_pitcher_content(x,y);
    fmn_hero_log_pitcher("pickup",content);
    fmn_state_set_item_count(FMN_ITEM_pitcher,content);
    if (content) {
      //TODO "ta da!" sound effect
      //TODO visual feedback, show what we picked up
    } else {
      //TODO repudiation sound effect
    }
  }
  //...either way, keep action nonzero and display the pitcher in "pour" position until button released.
}

/* Matches.
 */
 
static void fmn_hero_match_begin() {
  if (fmn_state_adjust_item_count(FMN_ITEM_match,-1)) {
    fmn_game_generate_light(FMN_HERO_MATCH_FIRE_TIME);
    fmn_hero.firetime=FMN_HERO_MATCH_FIRE_TIME;
    //TODO strike match sound effect
  } else {
    //TODO repudiation sound effect
  }
}

/* Corn.
 */
 
static void fmn_hero_corn_begin() {
  if (fmn_state_adjust_item_count(FMN_ITEM_corn,-1)) {
    struct fmn_sprite *corn=fmn_game_spawn_sprite(&fmn_sprite_type_corn,0,0,0,0);
    if (corn) {
      corn->x=fmn_hero.x;
      corn->y=fmn_hero.y;
      switch (fmn_hero.facedir) {
        case FMN_DIR_W: corn->x-=FMN_MM_PER_TILE; break;
        case FMN_DIR_E: corn->x+=FMN_MM_PER_TILE; break;
        case FMN_DIR_N: corn->y-=FMN_MM_PER_TILE; break;
        case FMN_DIR_S: corn->y+=FMN_MM_PER_TILE; break;
      }
    }
    //TODO sound effect
  } else {
    //TODO repudiation sound effect
    fmn_hero.action=-1;
  }
}

/* Shovel.
 */
 
static void fmn_hero_shovel_update() {
  if (fmn_hero.actiontime>60) {
    fmn_hero.action=-1;
  } else if (fmn_hero.actiontime==20) {
    int16_t x=fmn_hero.x,y=fmn_hero.y;
    switch (fmn_hero.facedir) {
      case FMN_DIR_W: x-=FMN_MM_PER_TILE>>1; break;
      case FMN_DIR_E: x+=FMN_MM_PER_TILE>>1; break;
      case FMN_DIR_N: y-=FMN_MM_PER_TILE>>1; break;
      case FMN_DIR_S: y+=FMN_MM_PER_TILE>>1; break;
    }
    if (
      fmn_map.tilesheet&&fmn_map.tilesheet->tileprops&&
      (x>=0)&&(y>=0)&&(x<FMN_COLC*FMN_MM_PER_TILE)&&(y<FMN_ROWC*FMN_MM_PER_TILE)
    ) {
      x/=FMN_MM_PER_TILE;
      y/=FMN_MM_PER_TILE;
      uint8_t tileid=fmn_map.v[y*FMN_COLC+x];
      if ((tileid==0x06)||(fmn_map.tilesheet->tileprops[tileid>>2]&(0xc0>>(tileid&3)))) {
        fprintf(stderr,"TODO shovel repudiation at %d,%d (0x%02x) %s:%d\n",x,y,tileid,__FILE__,__LINE__);
      } else {
        fprintf(stderr,"TODO dig up cell (%d,%d) tileid=0x%02x %s:%d\n",x,y,tileid,__FILE__,__LINE__);
        fmn_map.v[y*FMN_COLC+x]=0x06;
      }
    }
  }
}

static void fmn_hero_shovel_end() {
  fmn_hero.action=FMN_ITEM_shovel;
}

/* Coin.
 */
 
static void fmn_hero_coin_begin() {
  if (fmn_state_adjust_item_count(FMN_ITEM_coin,-1)) {
    int16_t x=fmn_hero.x,y=fmn_hero.y;
    int16_t tx=x,ty=y;
    switch (fmn_hero.facedir) {
      case FMN_DIR_N: y-=FMN_MM_PER_TILE>>1; ty-=FMN_MM_PER_TILE<<1; break;
      case FMN_DIR_S: y+=FMN_MM_PER_TILE>>1; ty+=FMN_MM_PER_TILE<<1; break;
      case FMN_DIR_W: x-=FMN_MM_PER_TILE>>1; tx-=FMN_MM_PER_TILE<<1; break;
      case FMN_DIR_E: x+=FMN_MM_PER_TILE>>1; tx+=FMN_MM_PER_TILE<<1; break;
    }
    struct fmn_sprite *coin=fmn_game_spawn_missile(x,y,tx,ty,FMN_MM_PER_TILE/4,0x1f);
  } else {
    //TODO repudiation sound effect
  }
}

/* Umbrella.
 */
 
static void fmn_hero_umbrella_end() {
  // We suspend direction changes while active.
  // Force a re-check.
  fmn_hero.action=-1;
  fmn_hero_event_dpad(fmn_hero.indx,fmn_hero.indy);
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
    case FMN_ITEM_shovel: fmn_hero_shovel_update(); break;
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
    case FMN_ITEM_chalk: fmn_hero_chalk_begin(); break;
    case FMN_ITEM_pitcher: fmn_hero_pitcher_begin(); break;
    case FMN_ITEM_coin: fmn_hero_coin_begin(); break;
    case FMN_ITEM_match: fmn_hero_match_begin(); break;
    case FMN_ITEM_corn: fmn_hero_corn_begin(); break;
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
    case FMN_ITEM_shovel: fmn_hero_shovel_end(); return; // sic return
    case FMN_ITEM_umbrella: fmn_hero_umbrella_end(); break;
  }
  fmn_hero.action=-1;
}

/* Dpad changed.
 */
 
static uint8_t fmn_hero_can_change_direction() {
  if (fmn_hero.action==FMN_ITEM_wand) return 0;
  if (fmn_hero.action==FMN_ITEM_umbrella) return 0;
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
