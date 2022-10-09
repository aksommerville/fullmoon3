#include "api/fmn_platform.h"
#include "game/fmn_game_mode.h"
#include "game/image/fmn_image.h"
#include "game/map/fmn_map.h"
#include "game/fmn_data.h"
#include "fmn_play.h"
#include <stdio.h>

/* Globals.
 */
 
#define FMN_PLAY_TRANSITION_TIME 30

struct fmn_map fmn_map={0};

static struct fmn_transition fmn_play_transition={0};

static uint8_t fmn_play_transition_bits_storage[(FMN_COLC*FMN_TILESIZE*FMN_ROWC*FMN_TILESIZE+3)>>2];
static struct fmn_image fmn_play_transition_bits={
//TODO Need some general solution for "image large enough to hold a framebuffer".
// This *must* not allocate more than needed, for Tiny.
// But *must* also support dynamic formats, for Linux.
  .v=fmn_play_transition_bits_storage,
  .w=FMN_COLC*FMN_TILESIZE,
  .h=FMN_ROWC*FMN_TILESIZE,
  .stride=(FMN_COLC*FMN_TILESIZE+3)>>2,
  .fmt=FMN_IMAGE_FMT_Y2,
  .flags=FMN_IMAGE_FLAG_WRITEABLE,
};

/* Map command at load.
 */
 
static int8_t fmn_game_cb_mapcmd_load(uint8_t cmd,const uint8_t *argv,uint8_t argc,void *userdata) {
  switch (cmd) {
    case FMN_MAP_CMD_TILESHEET: fmn_map.tilesheet=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_SONG: fmn_platform_audio_play_song(fmn_map.refv[argv[0]],0xffff); break;
    case FMN_MAP_CMD_HERO: break;//TODO
    case FMN_MAP_CMD_NEIGHBORW: fmn_map.neighborw=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_NEIGHBORE: fmn_map.neighbore=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_NEIGHBORN: fmn_map.neighborn=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_NEIGHBORS: fmn_map.neighbors=fmn_map.refv[argv[0]]; break;
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
  fmn_map_decode(&fmn_map,&fmnr_map_begin);
  fmn_map_for_each_command(fmn_map.cmdv,fmn_game_cb_mapcmd_load,0);
}

/* Navigate to map.
 */
 
uint8_t fmn_game_navigate(const struct fmn_map_resource *map,uint8_t transition) {
  if (!map) return 0;
  
  if (transition) {
    fmn_game_mode_play_render_pretransition(&fmn_play_transition_bits);
    fmn_play_transition.mode=transition;
    fmn_play_transition.p=0;
    fmn_play_transition.c=FMN_PLAY_TRANSITION_TIME;
    if (transition==FMN_TRANSITION_SPOTLIGHT) {
      fmn_play_transition.outx=10;//TODO hero position.
      fmn_play_transition.outy=6;
    }
  } else {
    fmn_play_transition.c=0;
  }
  
  //TODO exit hooks
  //TODO drop sprites etc
  
  fmn_map_decode(&fmn_map,map);
  fmn_map_for_each_command(fmn_map.cmdv,fmn_game_cb_mapcmd_load,0);
  
  if (transition==FMN_TRANSITION_SPOTLIGHT) {
    fmn_play_transition.inx=70;//TODO hero position.
    fmn_play_transition.iny=40;
  }
  
  return 1;
}

/* Input.
 */
 
void fmn_game_mode_play_input(uint8_t input,uint8_t pvinput) {

  //XXX TEMP: Navigate on dpad strokes.
  if ((input&FMN_BUTTON_LEFT)&&!(pvinput&FMN_BUTTON_LEFT)) fmn_game_navigate(fmn_map.neighborw,FMN_TRANSITION_PAN_LEFT);
  if ((input&FMN_BUTTON_RIGHT)&&!(pvinput&FMN_BUTTON_RIGHT)) fmn_game_navigate(fmn_map.neighbore,FMN_TRANSITION_PAN_RIGHT);
  if ((input&FMN_BUTTON_UP)&&!(pvinput&FMN_BUTTON_UP)) fmn_game_navigate(fmn_map.neighborn,FMN_TRANSITION_PAN_UP);
  if ((input&FMN_BUTTON_DOWN)&&!(pvinput&FMN_BUTTON_DOWN)) fmn_game_navigate(fmn_map.neighbors,FMN_TRANSITION_PAN_DOWN);
  
}

/* Update.
 */
 
void fmn_game_mode_play_update() {

  // Transition timing is based on update, not render. It's a gameplay thing.
  // (though those two timings should always be the same thing).
  if (fmn_play_transition.c) {
    fmn_play_transition.p++;
    if (fmn_play_transition.p>=fmn_play_transition.c) {
      fmn_play_transition.c=0;
    } else {
      // All normal updating is suspended while transition in progress.
      //TODO Maybe permit sprites to animate but not move or interact?
      //TODO Maybe watch inputs for cancellation of the transition? Not sure we can do much about that.
      return;
    }
  }

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
  //TODO Consider using a bgbits buffer. I think we can afford the RAM on Tiny, but only if it's needed for performance.
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
  if (!fmn_play_transition.c) return;
  fmn_image_transition(fb,&fmn_play_transition_bits,&fmn_play_transition);
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
  fmn_game_render_transition(fb);
  fmn_game_render_diegetic_overlay(fb);//TODO I'm on the fence, whether this is pre- or post- transition
  fmn_game_render_synthetic_overlay(fb);
}

void fmn_game_mode_play_render_pretransition(struct fmn_image *fb) {
  fmn_game_render_bg(fb);
  fmn_game_render_sprites(fb);
}
