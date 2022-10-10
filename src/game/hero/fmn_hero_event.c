#include "fmn_hero_internal.h"

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
  fmn_hero.ina=a;
  fmn_hero.inb=b;
}
