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

void fmn_game_create_soulballs(int16_t xmm,int16_t ymm);

/* (v) contains FMN_DIR_(W,E,N,S).
 * Returns nonzero if this is a real spell and we performed it.
 * btw a song is just a different set of spells; they work exactly the same way.
 * Songs may contain zeroes. Leading and trailing zeroes are ignored.
 */
uint8_t fmn_game_cast_spell(const uint8_t *v,uint8_t c);
uint8_t fmn_game_cast_song(const uint8_t *v,uint8_t c);

#endif
