#include "api/fmn_platform.h"
#include "game/image/fmn_image.h"
#include "game/fmn_data.h"
#include "game/fmn_game_mode.h"
#include "game/play/fmn_play.h"
#include <stdio.h>

/* Init.
 */

void fmn_game_mode_title_init() {
  fmn_platform_audio_play_song(fmnr_song_tangled_vine,fmnr_song_tangled_vine_length);
}

/* Input.
 */
 
void fmn_game_mode_title_input(uint8_t input,uint8_t pvinput) {
  if (
    ((input&FMN_BUTTON_A)&&!(pvinput&FMN_BUTTON_A))||
    ((input&FMN_BUTTON_B)&&!(pvinput&FMN_BUTTON_B))
  ) {
    fmn_game_reset();
    fmn_game_mode_set_play();
  }
}

/* Render.
 */
 
void fmn_game_mode_title_render(struct fmn_image *fb) {
  fmn_image_fill_rect(fb,0,0,fb->w,fb->h,0);
  
  // "Full Moon"
  fmn_image_blit(fb,(fb->w>>1)-(56>>1),(fb->h>>1)-(9>>1),&fmnr_image_uibits,0,0,56,9,0);
}
