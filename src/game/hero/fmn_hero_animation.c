#include "fmn_hero_internal.h"

/* Update animation.
 */
 
void fmn_hero_update_animation() {
  if (fmn_hero.indx||fmn_hero.indy) {
    if (fmn_hero.animclock) {
      fmn_hero.animclock--;
    } else {
      fmn_hero.animclock=6;
      fmn_hero.animframe++;
      if (fmn_hero.animframe>=4) fmn_hero.animframe=0;
    }
  } else {
    fmn_hero.animframe=0;
  }
}
