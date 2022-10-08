#include "api/fmn_platform.h"
#include "game/image/fmn_image.h"
#include "game/map/fmn_map.h"
#include "game/fmn_data.h"
#include "fmn_play.h"
#include <stdio.h>

struct fmn_map fmn_map={0};

/* Map command at load.
 */
 
static int8_t fmn_game_cb_mapcmd(uint8_t cmd,const uint8_t *argv,uint8_t argc,void *userdata) {
  switch (cmd) {
    case FMN_MAP_CMD_TILESHEET: fmn_map.tilesheet=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_SONG: fmn_platform_audio_play_song(fmn_map.refv[argv[0]],0xffff); break;
    case FMN_MAP_CMD_HERO: break;//TODO
    case FMN_MAP_CMD_EVENT1: break;//TODO
    case FMN_MAP_CMD_CELLIF: break;//TODO
    case FMN_MAP_CMD_SPRITE: break;//TODO
    case FMN_MAP_CMD_EVENTV: break;//TODO
  }
  return 0;
}

/* Reset.
 */
 
void fmn_game_reset() {
  fmn_map_decode(&fmn_map,fmnr_map_begin,fmnr_map_begin_refv);
  fmn_map_for_each_command(fmn_map.cmdv,fmn_game_cb_mapcmd,0);
}

/* Input.
 */
 
void fmn_game_mode_play_input(uint8_t input,uint8_t pvinput) {
}

/* Update.
 */
 
void fmn_game_mode_play_update() {
  //TODO
}

/* Render background.
 * This overwrites the whole framebuffer.
 */
 
static void fmn_game_render_bg(struct fmn_image *fb) {
  if (!fmn_map.tilesheet) {
    fmn_image_fill_rect(fb,0,0,fb->w,fb->h,0);
    return;
  }
  const uint8_t *src=fmn_map.v;
  int16_t dsty=0,yi=FMN_ROWC;
  for (;yi-->0;dsty+=FMN_TILESIZE) {
    int16_t dstx=0,xi=FMN_COLC;
    for (;xi-->0;dstx+=FMN_TILESIZE,src++) {
      int16_t srcx=((*src)&15)*FMN_TILESIZE;
      int16_t srcy=((*src)>>4)*FMN_TILESIZE;
      fmn_image_blit(fb,dstx,dsty,fmn_map.tilesheet,srcx,srcy,FMN_TILESIZE,FMN_TILESIZE,0);
    }
  }
}

/* Render sprites.
 */
 
static void fmn_game_render_sprites(struct fmn_image *fb) {
  //TODO sprites
}

/* Diegetic overlay.
 */
 
static void fmn_game_render_diegetic_overlay(struct fmn_image *fb) {
  //TODO rain
  // other effects?
}

/* Transition.
 * The new image is in (fb).
 */
 
static void fmn_game_render_transition(struct fmn_image *fb) {
  //TODO transition
}

/* Synthetic overlay.
 * This appears on top of the transition.
 */
 
static void fmn_game_render_synthetic_overlay(struct fmn_image *fb) {
  //TODO overlay
}

/* Render, TOC.
 */
 
void fmn_game_mode_play_render(struct fmn_image *fb) {
  fmn_game_render_bg(fb);
  fmn_game_render_sprites(fb);
  fmn_game_render_diegetic_overlay(fb);
  fmn_game_render_transition(fb);
  fmn_game_render_synthetic_overlay(fb);
}
