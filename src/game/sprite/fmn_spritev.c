#include "fmn_sprite.h"
#include <string.h>

struct fmn_sprite fmn_spritev[FMN_SPRITE_LIMIT];
uint8_t fmn_spritec=0;
int8_t fmn_sprite_sort_dir=1;

/* Compare sprites.
 */
 
static int8_t fmn_sprite_cmp(const struct fmn_sprite *a,const struct fmn_sprite *b) {
  if (!a->type) {
    if (!b->type) return 0;
    return 1;
  }
  if (!b->type) return -1;
  if (a->layer<b->layer) return -1;
  if (a->layer>b->layer) return 1;
  if (a->y<b->y) return -1;
  if (a->y>b->y) return 1;
  return 0;
}

/* Sort, one pass.
 */
 
void fmn_spritev_sort_partial() {
  if (fmn_spritec>1) {
    uint8_t first,last,i;
    if (fmn_sprite_sort_dir==1) {
      first=0;
      last=fmn_spritec-1;
    } else {
      first=fmn_spritec-1;
      last=0;
    }
    struct fmn_sprite *sprite=fmn_spritev+first;
    for (i=first;i!=last;i+=fmn_sprite_sort_dir,sprite+=fmn_sprite_sort_dir) {
      int8_t cmp=fmn_sprite_cmp(sprite,sprite+fmn_sprite_sort_dir);
      if (cmp==fmn_sprite_sort_dir) {
        struct fmn_sprite tmp=*sprite;
        *sprite=sprite[fmn_sprite_sort_dir];
        sprite[fmn_sprite_sort_dir]=tmp;
      }
    }
    fmn_sprite_sort_dir=-fmn_sprite_sort_dir;
  }
  while (fmn_spritec&&!fmn_spritev[fmn_spritec-1].type) fmn_spritec--;
}

/* Sort, full.
 */
 
void fmn_spritev_sort_full() {
  if (fmn_spritec>1) {
    uint8_t first=0,last=fmn_spritec-1,i;
    int d=1;
    while (first<last) {
      uint8_t done=1;
      struct fmn_sprite *sprite=fmn_spritev+first;
      for (i=first;i!=last;i+=d,sprite+=d) {
        int8_t cmp=fmn_sprite_cmp(sprite,sprite+fmn_sprite_sort_dir);
        if (cmp==d) {
          done=0;
          struct fmn_sprite tmp=*sprite;
          *sprite=sprite[d];
          sprite[d]=tmp;
        }
      }
      if (done) break;
      if (d==1) {
        last--;
        d=-1;
      } else {
        first++;
        d=1;
      }
    }
  }
  while (fmn_spritec&&!fmn_spritev[fmn_spritec-1].type) fmn_spritec--;
}

/* Get unused sprite.
 */

struct fmn_sprite *fmn_sprite_alloc() {
  if (fmn_spritec<FMN_SPRITE_LIMIT) {
    struct fmn_sprite *sprite=fmn_spritev+fmn_spritec++;
    memset(sprite,0,sizeof(struct fmn_sprite));
    return sprite;
  }
  struct fmn_sprite *sprite=fmn_spritev;
  uint8_t i=FMN_SPRITE_LIMIT;
  for (;i-->0;sprite++) {
    if (!sprite->type) {
      memset(sprite,0,sizeof(struct fmn_sprite));
      return sprite;
    }
  }
  return 0;
}

/* Clear list.
 */
 
void fmn_spritev_clear() {
  fmn_spritec=0;
}

/* Update sprites.
 */
 
void fmn_spritev_update() {
  struct fmn_sprite *sprite=fmn_spritev;
  uint8_t i=fmn_spritec;
  for (;i-->0;sprite++) {
    if (!sprite->type) continue;
    if (sprite->type->update) sprite->type->update(sprite);
  }
}
