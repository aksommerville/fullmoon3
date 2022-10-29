#include "fmn_hero_internal.h"

/* Update animation.
 */
 
static void fmn_hero_animate(uint8_t framelen,uint8_t framec) {
  if (fmn_hero.animclock) {
    fmn_hero.animclock--;
  } else {
    fmn_hero.animclock=framelen;
    fmn_hero.animframe++;
    if (fmn_hero.animframe>=framec) fmn_hero.animframe=0;
  }
}
 
void fmn_hero_update_animation() {
  if (fmn_hero.action==FMN_ITEM_violin) {
    fmn_hero_animate(6,4);
  } else if (fmn_hero.indx||fmn_hero.indy) {
    fmn_hero_animate(6,4);
  } else {
    fmn_hero.animframe=0;
  }
}
