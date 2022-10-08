/* fmn_play.h
 * Globals for the game state.
 * As opposed to UI state. Nothing in this interface should know about menus, etc.
 */
 
#ifndef FMN_PLAY_H
#define FMN_PLAY_H

struct fmn_map;

extern struct fmn_map fmn_map;

/* Reset game state to the beginning.
 * This does not change outer fmn_game_mode.
 */
void fmn_game_reset();

#endif
