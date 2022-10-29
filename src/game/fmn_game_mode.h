/* fmn_game_mode.h
 * This is the very top level, what kind of thing is the app doing right now?
 * Definitions and per-mode hooks, for fmn_game_main to call.
 */

#ifndef FMN_GAME_MODE_H
#define FMN_GAME_MODE_H

#include <stdint.h>

struct fmn_image;
struct fmn_sprite;

#define FMN_GAME_MODE_TITLE 1
#define FMN_GAME_MODE_PLAY 2
#define FMN_GAME_MODE_PAUSE 3
#define FMN_GAME_MODE_CHALK 4

/* Functions anyone can call, to change mode.
 */
void fmn_game_mode_set_title();
void fmn_game_mode_set_play();
void fmn_game_mode_set_pause();
void fmn_game_mode_set_chalk(struct fmn_sprite *chalkboard);

void fmn_game_mode_title_init();
void fmn_game_mode_title_input(uint8_t input,uint8_t pvinput);
void fmn_game_mode_title_render(struct fmn_image *fb);

void fmn_game_mode_play_input(uint8_t input,uint8_t pvinput);
void fmn_game_mode_play_update();
void fmn_game_mode_play_render(struct fmn_image *fb);
void fmn_game_mode_play_render_pretransition(struct fmn_image *fb);

void fmn_game_mode_pause_init();
void fmn_game_mode_pause_input(uint8_t input,uint8_t pvinput);
void fmn_game_mode_pause_update();
void fmn_game_mode_pause_render(struct fmn_image *fb);

void fmn_game_mode_chalk_init(struct fmn_sprite *chalkboard);
void fmn_game_mode_chalk_input(uint8_t input,uint8_t pvinput);
void fmn_game_mode_chalk_render(struct fmn_image *fb);

#endif
