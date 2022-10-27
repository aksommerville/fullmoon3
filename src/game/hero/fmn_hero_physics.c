#include "fmn_hero_internal.h"

/* Get physics for all cells under the hero.
 */
 
uint8_t fmn_hero_get_physics_underfoot() {
  if (!fmn_map.tilesheet) return 0;
  const uint8_t *tileprops=fmn_map.tilesheet->tileprops;
  if (!tileprops) return 0;
  int16_t l=fmn_hero.x-(FMN_MM_PER_TILE>>1);
  int16_t t=fmn_hero.y-(FMN_MM_PER_TILE>>1);
  int16_t r=l+FMN_MM_PER_TILE;
  int16_t b=t+FMN_MM_PER_TILE;
  int16_t cola=l/FMN_MM_PER_TILE; if (cola<0) cola=0;
  int16_t rowa=t/FMN_MM_PER_TILE; if (rowa<0) rowa=0;
  int16_t colz=(r-1)/FMN_MM_PER_TILE; if (colz>=FMN_COLC) colz=FMN_COLC-1;
  int16_t rowz=(b-1)/FMN_MM_PER_TILE; if (rowz>=FMN_ROWC) rowz=FMN_ROWC-1;
  if (cola>colz) return 0;
  if (rowa>rowz) return 0;
  uint8_t result=0;
  const uint8_t *cellrow=fmn_map.v+rowa*FMN_COLC+cola;
  for (;rowa<=rowz;rowa++,cellrow+=FMN_COLC) {
    const uint8_t *cellp=cellrow;
    int16_t col=cola;
    for (;col<=colz;col++,cellp++) {
      uint8_t prop=(tileprops[(*cellp)>>2]>>(6-(((*cellp)&3)<<1)))&3;
      result|=1<<prop;
    }
  }
  return result;
}

/* Escapement log.
 * Each entry is the positive distance you would have to move in each direction, to escape the collision.
 */
 
struct fmn_hero_escapement {
  int16_t w,e,n,s;
  struct fmn_sprite *pumpkin;
};

/* Collide against screen edges, if no neighbor map present in that direction.
 */
 
static uint8_t fmn_hero_collide_edges(
  struct fmn_hero_escapement *esc,
  int16_t l,int16_t t,int16_t r,int16_t b
) {
  uint8_t result=0;
  if ((l<0)&&!fmn_map.neighborw) {
    if (esc) {
      esc->w=esc->n=esc->s=0x7fff;
      esc->e=-l;
    }
    return 1;
  }
  if ((t<0)&&!fmn_map.neighborn) {
    if (esc) {
      esc->w=esc->e=esc->n=0x7fff;
      esc->s=-t;
    }
    return 1;
  }
  if ((r>FMN_COLC*FMN_MM_PER_TILE)&&!fmn_map.neighbore) {
    if (esc) {
      esc->n=esc->s=esc->e=0x7fff;
      esc->w=r-FMN_COLC*FMN_MM_PER_TILE;
    }
    return 1;
  }
  if ((b>FMN_ROWC*FMN_MM_PER_TILE)&&!fmn_map.neighbors) {
    if (esc) {
      esc->w=esc->e=esc->s=0x7fff;
      esc->n=b-FMN_ROWC*FMN_MM_PER_TILE;
    }
    return 1;
  }
  return 0;
}

/* Collide against solid cells of the grid.
 */
 
static uint8_t fmn_hero_collide_grid(
  struct fmn_hero_escapement *esc,
  int16_t l,int16_t t,int16_t r,int16_t b
) {
  if (!fmn_map.tilesheet) return 0;
  const uint8_t *tileprops=fmn_map.tilesheet->tileprops;
  if (!tileprops) return 0;
  int16_t cola=l/FMN_MM_PER_TILE; if (cola<0) cola=0;
  int16_t rowa=t/FMN_MM_PER_TILE; if (rowa<0) rowa=0;
  int16_t colz=(r-1)/FMN_MM_PER_TILE; if (colz>=FMN_COLC) colz=FMN_COLC-1;
  int16_t rowz=(b-1)/FMN_MM_PER_TILE; if (rowz>=FMN_ROWC) rowz=FMN_ROWC-1;
  if (cola>colz) return 0;
  if (rowa>rowz) return 0;
  uint8_t result=0;
  const uint8_t *cellrow=fmn_map.v+rowa*FMN_COLC+cola;
  for (;rowa<=rowz;rowa++,cellrow+=FMN_COLC) {
    const uint8_t *cellp=cellrow;
    int16_t col=cola;
    for (;col<=colz;col++,cellp++) {
      uint8_t prop=(tileprops[(*cellp)>>2]>>(6-(((*cellp)&3)<<1)))&3;
      switch (prop) {
        case 0: continue; // vacant
        case 1: break; // solid
        case 2: if (fmn_hero.action==FMN_ITEM_broom) continue; break; // hole
        case 3: break; // reserved ...?
      }
      if (!esc) return 1;
      
      int16_t celll=col*FMN_MM_PER_TILE;
      int16_t cellt=rowa*FMN_MM_PER_TILE;
      int16_t cellr=celll+FMN_MM_PER_TILE;
      int16_t cellb=cellt+FMN_MM_PER_TILE;
      int16_t q;
      if ((q=r-celll)>esc->w) esc->w=q;
      if ((q=b-cellt)>esc->n) esc->n=q;
      if ((q=cellr-l)>esc->e) esc->e=q;
      if ((q=cellb-t)>esc->s) esc->s=q;
      result=1;
    }
  }
  return result;
}

/* Collide against solid sprites.
 */
 
static uint8_t fmn_hero_collide_sprites(
  struct fmn_hero_escapement *esc,
  int16_t l,int16_t t,int16_t r,int16_t b
) {
  uint8_t result=0;
  struct fmn_sprite *sprite=fmn_spritev;
  uint8_t i=fmn_spritec;
  for (;i-->0;sprite++) {
    if (!sprite->type) continue;
    if (sprite->type==&fmn_sprite_type_hero) continue;
    if (!(sprite->flags&FMN_SPRITE_FLAG_SOLID)) continue;
    
    int16_t sl,st,sw,sh;
    fmn_sprite_hitbox(&sl,&st,&sw,&sh,sprite);
    if (sl>=r) continue;
    if (st>=b) continue;
    int16_t sr=sl+sw; if (sr<=l) continue;
    int16_t sb=st+sh; if (sb<=t) continue;
    
    if (!esc) return 1;
    esc->pumpkin=sprite;
    int16_t q;
    if ((q=r-sl)>esc->w) esc->w=q;
    if ((q=b-st)>esc->n) esc->n=q;
    if ((q=sr-l)>esc->e) esc->e=q;
    if ((q=sb-t)>esc->s) esc->s=q;
    result=1;
  }
  return result;
}

/* Detect collisions.
 * Nonzero if a collision exists.
 * If supplied, and we return nonzero, we populate (esc) with our best* guess of escape distance in each direction.
 * [*] NB our "best" is not perfect.
 * An escape distance of 0x7fff means we're confident there's no valid escape that way.
 */
 
static uint8_t fmn_hero_detect_collisions(
  struct fmn_hero_escapement *esc,
  int16_t x,int16_t y,int16_t w,int16_t h
) {
  int16_t r=x+w,b=y+h;
  
  uint8_t result=fmn_hero_collide_edges(esc,x,y,r,b);
  if (result&&!esc) return 1;
  
  if (fmn_hero_collide_grid(esc,x,y,r,b)) {
    if (!esc) return 1;
    result=1;
  }
  
  if (fmn_hero_collide_sprites(esc,x,y,r,b)) {
    if (!esc) return 1;
    result=1;
  }
  
  return result;
}

/* Move with physics, main entry point.
 */

int16_t fmn_hero_move_with_physics(
  struct fmn_sprite **pumpkin,
  int16_t dx,int16_t dy
) {

  // Where are we, before and after the move?
  //TODO proper hit box
  int16_t ox=fmn_hero.x-(FMN_MM_PER_TILE>>1);
  int16_t oy=fmn_hero.y-(FMN_MM_PER_TILE>>1);
  int16_t nx=ox+dx;
  int16_t ny=oy+dy;
  int16_t w=FMN_MM_PER_TILE;
  int16_t h=FMN_MM_PER_TILE;

  // Check all collisions and receive the four escapements.
  // No collision? Cool. Apply it and get out.
  struct fmn_hero_escapement esc={0};
  if (!fmn_hero_detect_collisions(&esc,nx,ny,w,h)) {
    fmn_hero.x+=dx;
    fmn_hero.y+=dy;
    return ((dx<0)?-dx:dx)+((dy<0)?-dy:dy);
  }
  if (pumpkin) *pumpkin=esc.pumpkin;
  
  // Neuter any escape with a negative value, or greater than the original move.
  int16_t limit=((dx<0)?-dx:dx)+((dy<0)?-dy:dy);
  if ((esc.w<0)||(esc.w>limit)) esc.w=0x7fff;
  if ((esc.e<0)||(esc.e>limit)) esc.e=0x7fff;
  if ((esc.n<0)||(esc.n>limit)) esc.n=0x7fff;
  if ((esc.s<0)||(esc.s>limit)) esc.s=0x7fff;
  
  // Sort the four escapes by distance low to high.
  struct fmn_hero_motion_candidate {
    int16_t escd;
    int16_t dx,dy;
  } candidatev[4]={
    {esc.w,-1,0},
    {esc.e,1,0},
    {esc.n,0,-1},
    {esc.s,0,1},
  };
  #define CMP(pa,pb) if (candidatev[pa].escd>candidatev[pb].escd) { \
    struct fmn_hero_motion_candidate tmp=candidatev[pa]; \
    candidatev[pa]=candidatev[pb]; \
    candidatev[pb]=tmp; \
  }
  CMP(0,1)
  CMP(1,2)
  CMP(2,3)
  CMP(1,2)
  CMP(0,1)
  CMP(1,2)
  #undef CMP
  
  // Retest each escaped position. The first to yield a truly valid position, go with it.
  const struct fmn_hero_motion_candidate *candidate=candidatev;
  uint8_t ci=4; for (;ci-->0;candidate++) {
    if (candidate->escd>=0x7fff) break; // definitely impossible
    int16_t qx=nx+candidate->escd*candidate->dx;
    int16_t qy=ny+candidate->escd*candidate->dy;
    if (fmn_hero_detect_collisions(0,qx,qy,w,h)) continue;
    dx=qx-ox;
    dy=qy-oy;
    fmn_hero.x+=dx;
    fmn_hero.y+=dy;
    return ((dx<0)?-dx:dx)+((dy<0)?-dy:dy);
  }
  
  // No escape proved valid. Reject all motion.
  // This happens often when you walk diagonally into a corner.
  // I'm not too worried about it.
  return 0;
}
