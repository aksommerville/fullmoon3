#include "fmn_hero_internal.h"

/* Trigger navigation if we stepped off a screen edge.
 */
 
static uint8_t fmn_hero_check_edge_navigation() {
  if (fmn_hero.x<0) {
    if (fmn_game_navigate(fmn_map.neighborw,0,0,FMN_TRANSITION_PAN_LEFT)) return 1;
  } else if (fmn_hero.x>=FMN_COLC*FMN_MM_PER_TILE) {
    if (fmn_game_navigate(fmn_map.neighbore,0,0,FMN_TRANSITION_PAN_RIGHT)) return 1;
  }
  if (fmn_hero.y<0) {
    if (fmn_game_navigate(fmn_map.neighborn,0,0,FMN_TRANSITION_PAN_UP)) return 1;
  } else if (fmn_hero.y>=FMN_ROWC*FMN_MM_PER_TILE) {
    if (fmn_game_navigate(fmn_map.neighbors,0,0,FMN_TRANSITION_PAN_DOWN)) return 1;
  }
  return 0;
}

/* Check current cell position.
 * Trigger any pertinent activity if changed.
 * Returns nonzero if we navigate.
 */
 
static uint8_t fmn_hero_exit_cell(int8_t col,int8_t row) {
  if ((col<0)||(row<0)||(col>=FMN_COLC)||(row>=FMN_ROWC)) return 0;
  //TODO exit-cell hooks
  return 0;
}

static int8_t fmn_hero_enter_cell_cb(uint8_t cmd,const uint8_t *argv,uint8_t argc,void *userdata) {
  switch (cmd) {
    case FMN_MAP_CMD_DOOR: {
        uint8_t srcx=argv[0]>>4;
        if (srcx!=fmn_hero.col) return 0;
        uint8_t srcy=argv[0]&15;
        if (srcy!=fmn_hero.row) return 0;
        uint8_t dstx=argv[2]>>4;
        uint8_t dsty=argv[2]&15;
        if (!fmn_game_navigate(fmn_map.refv[argv[1]],dstx,dsty,FMN_TRANSITION_SPOTLIGHT)) return 0;
        return 1;
      }
  }
  return 0;
}

static uint8_t fmn_hero_enter_cell(int8_t col,int8_t row) {
  if ((col<0)||(row<0)||(col>=FMN_COLC)||(row>=FMN_ROWC)) return 0;
  //TODO Do these care whether we're riding the broom?
  if (fmn_map.has_hero_cell_features) {
    return fmn_map_for_each_command(fmn_map.cmdv,fmn_hero_enter_cell_cb,0);
  }
  return 0;
}
 
static uint8_t fmn_hero_check_cells() {
  int8_t col=fmn_hero.x/FMN_MM_PER_TILE;
  int8_t row=fmn_hero.y/FMN_MM_PER_TILE;
  if ((col!=fmn_hero.col)||(row!=fmn_hero.row)) {
    fmn_hero_exit_cell(fmn_hero.col,fmn_hero.row);
    fmn_hero.col=col;
    fmn_hero.row=row;
    return fmn_hero_enter_cell(col,row);
  }
  return 0;
}

/* Cross-axis correction.
 * Call when the player requests motion on just one axis and is completely stymied.
 * If the other axis is not aligned to a half-tile, we nudge toward the nearest half-tile interval.
 * This is more important than it sounds. Without this, it's nearly impossible to enter a witch-wide gap.
 * And it also bears on speed-running: Since the nudge speed is so low, it's advantageous to hit gaps precisely.
 */
 
#define FMN_HERO_NUDGE_INTERVAL (FMN_MM_PER_TILE)
#define FMN_HERO_NUDGE_SPEED 10
 
static uint8_t fmn_hero_nudge(int16_t *v) {
  //TODO We're bypassing collision detection here. Is that OK?
  int16_t mod=((*v)-(FMN_MM_PER_TILE>>1))%FMN_HERO_NUDGE_INTERVAL;
  if (!mod) return 0;
  if (mod>=FMN_HERO_NUDGE_INTERVAL>>1) {
    if (mod>FMN_HERO_NUDGE_INTERVAL-FMN_HERO_NUDGE_SPEED) (*v)+=FMN_HERO_NUDGE_INTERVAL-mod;
    else (*v)+=FMN_HERO_NUDGE_SPEED;
  } else {
    if (mod<FMN_HERO_NUDGE_SPEED) (*v)-=mod;
    else (*v)-=FMN_HERO_NUDGE_SPEED;
  }
  return 1;
}

/* Target speed and acceleration.
 * TODO Slippery floors, that kind of thing?
 * Note that deceleration is positive.
 */
 
static int16_t fmn_hero_get_target_speed() {
  if (fmn_hero.action==FMN_ITEM_broom) return FMN_MM_PER_TILE/5;
  return FMN_MM_PER_TILE/10;
}

static int16_t fmn_hero_get_acceleration() {
  if (fmn_hero.action==FMN_ITEM_broom) return 25;
  return 30;
}

static int16_t fmn_hero_get_deceleration() {
  if (fmn_hero.action==FMN_ITEM_broom) return 10;
  return 20;
}

/* Walk, update.
 */

void fmn_hero_update_walk() {

  // Get out if motion is suppressed due to an action.
  switch (fmn_hero.action) {
    case FMN_ITEM_wand:
    case FMN_ITEM_violin:
    case FMN_ITEM_shovel:
      return;
  }

  // Accelerate or decelerate.
  if (fmn_hero.indx||fmn_hero.indy) {
    fmn_hero.walkdx=fmn_hero.indx;
    fmn_hero.walkdy=fmn_hero.indy;
    int16_t target=fmn_hero_get_target_speed();
    if (fmn_hero.walkspeed<target) {
      int16_t accel=fmn_hero_get_acceleration();
      fmn_hero.walkspeed+=accel;
      if (fmn_hero.walkspeed>target) fmn_hero.walkspeed=target;
    } else if (fmn_hero.walkspeed>target) {
      int16_t decel=fmn_hero_get_deceleration();
      fmn_hero.walkspeed-=decel;
      if (fmn_hero.walkspeed<target) fmn_hero.walkspeed=target;
    }
  } else if (fmn_hero.walkspeed) {
    int16_t decel=fmn_hero_get_deceleration();
    fmn_hero.walkspeed-=decel;
    if (fmn_hero.walkspeed<=0) {
      fmn_hero.walkspeed=0;
      return;
    }
  } else {
    fmn_hero.pushable=0;
    fmn_hero.pushcounter=0;
    return;
  }

  struct fmn_sprite *pumpkin=0;
  if (!fmn_hero_move_with_physics(
    &pumpkin,
    fmn_hero.walkspeed*fmn_hero.walkdx,
    fmn_hero.walkspeed*fmn_hero.walkdy
  )) {
    uint8_t nudged=0;
    if (!fmn_hero.indx&&fmn_hero.indy) nudged=fmn_hero_nudge(&fmn_hero.x);
    else if (fmn_hero.indx&&!fmn_hero.indy) nudged=fmn_hero_nudge(&fmn_hero.y);
    if (pumpkin&&pumpkin->type->push) {
      if (pumpkin==fmn_hero.pushable) {
        if (fmn_hero.pushcounter>=FMN_HERO_PUSH_DELAY) {
          pumpkin->type->push(pumpkin,fmn_hero.walkdx,fmn_hero.walkdy);
          fmn_hero.pushcounter=0;
        } else {
          fmn_hero.pushcounter++;
        }
      } else {
        fmn_hero.pushable=pumpkin;
        fmn_hero.pushcounter=0;
      }
    } else {
      fmn_hero.pushable=0;
      fmn_hero.pushcounter=0;
    }
    return;
  }
  fmn_hero.pushable=0;
  fmn_hero.pushcounter=0;
  
  if (fmn_hero_check_edge_navigation()) return;
  if (fmn_hero_check_cells()) return;
}
