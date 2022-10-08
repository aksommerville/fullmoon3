#include "api/fmn_platform.h"
#include "api/fmn_client.h"
#include "game/image/fmn_image.h"
#include "game/play/fmn_play.h"
#include "fmn_data.h"
#include "fmn_game_mode.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

static uint8_t pvinput=0;
static uint8_t fmn_game_mode=0;

/* Mode changes.
 */
 
void fmn_game_mode_set_title() {
  if (fmn_game_mode==FMN_GAME_MODE_TITLE) return;
  fmn_game_mode=FMN_GAME_MODE_TITLE;
  fmn_game_mode_title_init();
}

void fmn_game_mode_set_play() {
  if (fmn_game_mode==FMN_GAME_MODE_PLAY) return;
  fmn_game_mode=FMN_GAME_MODE_PLAY;
  // Do not fmn_game_reset() here. Changing mode might mean like returning from the pause screen.
}

/* Setup.
 */

void setup() {
  if (fmn_platform_init()<0) return;
  fmn_game_mode_set_title();
}

/* Loop.
 */

void loop() {
  uint8_t input=fmn_platform_update();
  if (input!=pvinput) {
  
    switch (fmn_game_mode) {
      case FMN_GAME_MODE_TITLE: fmn_game_mode_title_input(input,pvinput); break;
      case FMN_GAME_MODE_PLAY: fmn_game_mode_play_input(input,pvinput); break;
    }
    
    pvinput=input;
  }
  
  switch (fmn_game_mode) {
    case FMN_GAME_MODE_TITLE: break;
    case FMN_GAME_MODE_PLAY: fmn_game_mode_play_update(); break;
  }
  
  struct fmn_image *fb=fmn_platform_video_begin();
  if (fb) {
  
    switch (fmn_game_mode) {
      case FMN_GAME_MODE_TITLE: fmn_game_mode_title_render(fb); break;
      case FMN_GAME_MODE_PLAY: fmn_game_mode_play_render(fb); break;
    }
    
    fmn_platform_video_end(fb);
  }
}
