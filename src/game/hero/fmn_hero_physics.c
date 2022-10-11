#include "fmn_hero_internal.h"

/* Nonzero if any cell touching the box is impassable.
 * OOB cells are presumed passable.
 */
 
static uint8_t fmn_hero_grid_impassable(int16_t x,int16_t y,int16_t w,int16_t h) {
  if (!fmn_map.tilesheet) return 0;
  const uint8_t *tileprops=fmn_map.tilesheet->tileprops;
  if (!tileprops) return 0;
  int16_t cola=x/FMN_MM_PER_TILE;
  int16_t rowa=y/FMN_MM_PER_TILE;
  int16_t colz=(x+w-1)/FMN_MM_PER_TILE;
  int16_t rowz=(y+h-1)/FMN_MM_PER_TILE;
  //fprintf(stderr,"%s (%d,%d,%d,%d) col=%d..%d row=%d..%d\n",__func__,x,y,w,h,cola,colz,rowa,rowz);
  if (cola<0) cola=0;
  if (rowa<0) rowa=0;
  if (colz>=FMN_COLC) colz=FMN_COLC-1;
  if (rowz>=FMN_ROWC) rowz=FMN_ROWC-1;
  if (cola>colz) return 0;
  if (rowa>rowz) return 0;
  const uint8_t *cellrow=fmn_map.v+rowa*FMN_COLC+cola;
  for (;rowa<=rowz;rowa++,cellrow+=FMN_COLC) {
    const uint8_t *cellp=cellrow;
    int16_t col=cola;
    for (;col<=colz;col++,cellp++) {
      uint8_t prop=(tileprops[(*cellp)>>2]>>(6-(((*cellp)&3)<<1)))&3;
      switch (prop) {
        case 0: break; // vacant
        case 1: return 1; // solid
        case 2: return 1; // hole TODO flying
        case 3: return 1; // reserved ...?
      }
    }
  }
  return 0;
}

/* Look for solid sprite collisions in the given box, and return clearance from the furthest sprite edge to the given box edge.
 * Never negative.
 */
 
static int16_t fmn_hero_check_solid_sprite_collisions(int16_t x,int16_t y,int16_t w,int16_t h,uint8_t edge) {
  int16_t r=x+w,b=y+h,clearance=0x7fff;
  const struct fmn_sprite *sprite=fmn_spritev;
  uint8_t i=fmn_spritec;
  for (;i-->0;sprite++) {
    if (!sprite->type) continue;
    if (sprite->type==&fmn_sprite_type_hero) continue;
    //TODO "solid" flag for sprites
    //TODO proper hitbox for sprite
    int16_t sl=sprite->x-(FMN_MM_PER_TILE>>1);
    if (sl>=r) continue;
    int16_t st=sprite->y-(FMN_MM_PER_TILE>>1);
    if (st>=b) continue;
    int16_t sr=sl+FMN_MM_PER_TILE;
    if (sr<=x) continue;
    int16_t sb=st+FMN_MM_PER_TILE;
    if (sb<=y) continue;
    int16_t q=0;
    switch (edge) {
      case FMN_DIR_N: q=st-y; break;
      case FMN_DIR_S: q=b-sb; break;
      case FMN_DIR_W: q=sl-x; break;
      case FMN_DIR_E: q=r-sr; break;
    }
    if (q<=0) return 0;
    if (q<clearance) clearance=q;
  }
  return clearance;
}

/* Check how far we can move in one direction.
 */
 
static int16_t fmn_hero_measure_movement_left(int16_t x,int16_t y,int16_t w,int16_t h,int16_t d) {
  int16_t available=0x7fff;
  if (!fmn_map.neighborw) {
    if ((available=x)<1) return 0;
  }
  if (fmn_hero_grid_impassable(x-d,y,d,h)) { // assumes (d) no larger than one tile
    if ((available=(x+w)%FMN_MM_PER_TILE)<1) return 0;
  }
  if (available>d) available=d;
  int16_t q=fmn_hero_check_solid_sprite_collisions(x-available,y,available,h,FMN_DIR_E);
  if (q<available) available=q;
  return available;
}
 
static int16_t fmn_hero_measure_movement_right(int16_t x,int16_t y,int16_t w,int16_t h,int16_t d) {
  int16_t available=0x7fff;
  if (!fmn_map.neighbore) {
    if ((available=FMN_COLC*FMN_MM_PER_TILE-w-x)<1) return 0;
  }
  if (fmn_hero_grid_impassable(x+w,y,d,h)) { // assumes (d) no larger than one tile
    int16_t sub=x%FMN_MM_PER_TILE;
    if (!sub) return 0;
    available=FMN_MM_PER_TILE-sub;
  }
  if (available>d) available=d;
  int16_t q=fmn_hero_check_solid_sprite_collisions(x+w,y,available,h,FMN_DIR_W);
  if (q<available) available=q;
  return available;
}
 
static int16_t fmn_hero_measure_movement_up(int16_t x,int16_t y,int16_t w,int16_t h,int16_t d) {
  int16_t available=0x7fff;
  if (!fmn_map.neighborn) {
    if ((available=y)<1) return 0;
  }
  if (fmn_hero_grid_impassable(x,y-d,w,d)) { // assumes (d) no larger than one tile
    if ((available=(y+h)%FMN_MM_PER_TILE)<1) return 0;
  }
  if (available>d) available=d;
  int16_t q=fmn_hero_check_solid_sprite_collisions(x,y-available,w,available,FMN_DIR_S);
  if (q<available) available=q;
  return available;
}
 
static int16_t fmn_hero_measure_movement_down(int16_t x,int16_t y,int16_t w,int16_t h,int16_t d) {
  int16_t available=0x7fff;
  if (!fmn_map.neighbors) {
    if ((available=FMN_ROWC*FMN_MM_PER_TILE-h-y)<1) return 0;
  }
  if (fmn_hero_grid_impassable(x,y+h,w,d)) { // assumes (d) no larger than one tile
    int16_t sub=y%FMN_MM_PER_TILE;
    if (!sub) return 0;
    available=FMN_MM_PER_TILE-sub;
  }
  if (available>d) available=d;
  int16_t q=fmn_hero_check_solid_sprite_collisions(x,y+h,w,available,FMN_DIR_N);
  if (q<available) available=q;
  return available;
}

/* Move with physics, main entry point.
 */
 
int16_t fmn_hero_move_with_physics(int16_t dx,int16_t dy) {

  // We need exactly one axis to be nonzero. Take care of other cases.
  if (!dx&&!dy) return 0;
  if (dx&&dy) {
    return fmn_hero_move_with_physics(dx,0)+fmn_hero_move_with_physics(0,dy);
  }
  
  // Where are we, before moving?
  //TODO proper hit box
  int16_t x=fmn_hero.x-(FMN_MM_PER_TILE>>1);
  int16_t y=fmn_hero.y-(FMN_MM_PER_TILE>>1);
  int16_t w=FMN_MM_PER_TILE;
  int16_t h=FMN_MM_PER_TILE;
  
  // Determine how far we can move in the requested direction, and go no further.
  // Might as well check all 4 possibilities independently.
  if (dx<0) {
    int16_t available=fmn_hero_measure_movement_left(x,y,w,h,-dx);
    if (available<1) return 0;
    if (available<-dx) dx=-available;
  } else if (dx>0) {
    int16_t available=fmn_hero_measure_movement_right(x,y,w,h,dx);
    if (available<1) return 0;
    if (available<dx) dx=available;
  } else if (dy<0) {
    int16_t available=fmn_hero_measure_movement_up(x,y,w,h,-dy);
    if (available<1) return 0;
    if (available<-dy) dy=-available;
  } else {
    int16_t available=fmn_hero_measure_movement_down(x,y,w,h,dy);
    if (available<1) return 0;
    if (available<dy) dy=available;
  }
  
  // Done, apply it.
  fmn_hero.x+=dx;
  fmn_hero.y+=dy;
  if (dx<0) return -dx;
  if (dy<0) return -dy;
  if (dx) return dx;
  return dy;
}
