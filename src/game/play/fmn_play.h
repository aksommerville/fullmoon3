/* fmn_play.h
 * Globals for the game state.
 * As opposed to UI state. Nothing in this interface should know about menus, etc.
 */
 
#ifndef FMN_PLAY_H
#define FMN_PLAY_H

struct fmn_map;
struct fmn_sprite;
struct fmn_sprite_type;

extern struct fmn_map fmn_map;

/* Reset game state to the beginning.
 * This does not change outer fmn_game_mode.
 */
void fmn_game_reset();

struct fmn_sprite *fmn_game_spawn_sprite(
  const struct fmn_sprite_type *type,
  uint8_t col,uint8_t row,
  const uint8_t *argv,uint8_t argc
);

#endif
