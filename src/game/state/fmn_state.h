/* fmn_state.h
 * Authority for the state of gameplay, at an abstract level.
 * Responsible for persistence and passwords, or whatever we're going to do there.
 */
 
#ifndef FMN_STATE_H
#define FMN_STATE_H

#include <stdint.h>

/* There are 16 possible items, each with a "possessed" flag, an optional 0..254 count, and one selected at all times.
 * Unpossessed items always have a zero or infinite count, additions will be rejected.
 * It is possible that the selected item is not possessed, in which case the hero is unarmed.
 * (might want to restrict to 15 items in design, to keep "unarmed" possible?)
 */
void fmn_state_reset_items();
uint8_t fmn_state_get_selected_item(); // => itemid 0..15
void fmn_state_set_selected_item(uint8_t itemid);
uint16_t fmn_state_get_possessed_items(); // => bitfields, 1<<itemid
void fmn_state_add_possessed_item(uint8_t itemid); // Possession is forever; no "remove"
uint8_t fmn_state_get_item_count(uint8_t itemid); // => 255 if uncounted, 0..254 otherwise
uint8_t fmn_state_adjust_item_count(uint8_t itemid,int8_t d); // => nonzero if changed; we manage clamping etc
uint8_t fmn_state_get_selected_item_if_possessed(); // 0..15; or 255 if not possessed
// itemid:
#define FMN_ITEM_broom       0
#define FMN_ITEM_feather     1
#define FMN_ITEM_wand        2
#define FMN_ITEM_violin      3
#define FMN_ITEM_bell        4
#define FMN_ITEM_chalk       5
#define FMN_ITEM_pitcher     6 /* "count" is actually an enum for the pitcher's contents */
#define FMN_ITEM_coin        7 /* counted */
#define FMN_ITEM_match       8 /* counted */
#define FMN_ITEM_corn        9 /* counted */
#define FMN_ITEM_umbrella   10
#define FMN_ITEM_shovel     11
#define FMN_ITEM_compass    12
#define FMN_ITEM_todo1      13
#define FMN_ITEM_todo2      14
// 15 is available but try not to use it, so there's always a "no item", and we can display as 5x3.
#define FMN_FOR_EACH_ITEM \
  _(broom) \
  _(feather) \
  _(wand) \
  _(violin) \
  _(bell) \
  _(chalk) \
  _(pitcher) \
  _(coin) \
  _(match) \
  _(corn) \
  _(umbrella) \
  _(shovel) \
  _(compass) \
  _(todo1) \
  _(todo2)

#endif
