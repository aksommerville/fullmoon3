#include "fmn_hero_internal.h"

struct fmn_hero fmn_hero={0};

/* Reset.
 */
 
void fmn_hero_reset() {
  memset(&fmn_hero,0,sizeof(fmn_hero));
  fmn_hero.x=(FMN_COLC*FMN_MM_PER_TILE)>>1;
  fmn_hero.y=(FMN_ROWC*FMN_MM_PER_TILE)>>1;
  fmn_hero.col=fmn_hero.row=-1;
  fmn_hero.facedir=FMN_DIR_E;
  fmn_hero.action=-1;
}

/* Trivial public accessors.
 */
 
void fmn_hero_get_position(int16_t *xmm,int16_t *ymm) {
  *xmm=fmn_hero.x;
  *ymm=fmn_hero.y;
}

void fmn_hero_set_position(int16_t xmm,int16_t ymm) {
  fmn_hero.x=xmm;
  fmn_hero.y=ymm;
  //fmn_hero.col=fmn_hero.row=-1;
  fmn_hero.col=xmm/FMN_MM_PER_TILE;
  fmn_hero.row=ymm/FMN_MM_PER_TILE;
}

/* Input changed.
 */
 
void fmn_hero_input(uint8_t input,uint8_t pvinput) {
  int8_t indx=0,indy=0;
  uint8_t ina=0,inb=0;
  switch (input&(FMN_BUTTON_LEFT|FMN_BUTTON_RIGHT)) {
    case FMN_BUTTON_LEFT: indx=-1; break;
    case FMN_BUTTON_RIGHT: indx=1; break;
  }
  switch (input&(FMN_BUTTON_UP|FMN_BUTTON_DOWN)) {
    case FMN_BUTTON_UP: indy=-1; break;
    case FMN_BUTTON_DOWN: indy=1; break;
  }
  if (input&FMN_BUTTON_A) ina=1;
  if (input&FMN_BUTTON_B) inb=1;
  if ((indx!=fmn_hero.indx)||(indy!=fmn_hero.indy)) {
    fmn_hero_event_dpad(indx,indy);
  }
  if ((ina!=fmn_hero.ina)||(inb!=fmn_hero.inb)) {
    fmn_hero_event_buttons(ina,inb);
  }
}

/* Update.
 */
 
void fmn_hero_update() {
  fmn_hero_update_walk();
  fmn_hero_update_animation();
  fmn_hero_update_action();
}
