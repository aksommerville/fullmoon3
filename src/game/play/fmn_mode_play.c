#include "api/fmn_platform.h"
#include "game/fmn_game_mode.h"
#include "game/image/fmn_image.h"
#include "game/map/fmn_map.h"
#include "game/hero/fmn_hero.h"
#include "game/sprite/fmn_sprite.h"
#include "game/state/fmn_state.h"
#include "game/violin/fmn_violin.h"
#include "game/fmn_data.h"
#include "fmn_play.h"
#include <stdio.h>

/* Globals.
 */
 
#define FMN_PLAY_TRANSITION_TIME 30

struct fmn_map fmn_map={0};

static uint16_t fmn_play_light_time=0;

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
 
struct fmn_game_cb_mapcmd_load_context {
  uint8_t herox,heroy; // Default position per HERO command.
};
 
static int8_t fmn_game_cb_mapcmd_load(uint8_t cmd,const uint8_t *argv,uint8_t argc,void *userdata) {
  struct fmn_game_cb_mapcmd_load_context *ctx=userdata;
  switch (cmd) {
    case FMN_MAP_CMD_TILESHEET: fmn_map.tilesheet=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_SONG: fmn_platform_audio_play_song(fmn_map.refv[argv[0]],0xffff); break;
    case FMN_MAP_CMD_HERO: ctx->herox=argv[0]>>4; ctx->heroy=argv[0]&15; break;
    case FMN_MAP_CMD_NEIGHBORW: fmn_map.neighborw=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_NEIGHBORE: fmn_map.neighbore=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_NEIGHBORN: fmn_map.neighborn=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_NEIGHBORS: fmn_map.neighbors=fmn_map.refv[argv[0]]; break;
    case FMN_MAP_CMD_EVENT1: break;//TODO
    case FMN_MAP_CMD_CELLIF: break;//TODO
    case FMN_MAP_CMD_DOOR: fmn_map.has_hero_cell_features=1; break;
    case FMN_MAP_CMD_SPRITE: fmn_game_spawn_sprite(fmn_map.refv[argv[1]],argv[0]>>4,argv[0]&15,argv+2,argc-2); break;
    case FMN_MAP_CMD_EVENTV: break;//TODO
  }
  return 0;
}

/* Reset.
 */
 
void fmn_game_reset() {
  fmn_hero_reset();
  fmn_spritev_clear();
  fmn_state_reset_items();
  fmn_play_light_time=0;
  
  fmn_map_decode(&fmn_map,&fmnr_map_begin);
  struct fmn_game_cb_mapcmd_load_context ctx={0};
  fmn_map_for_each_command(fmn_map.cmdv,fmn_game_cb_mapcmd_load,&ctx);
  if (ctx.herox||ctx.heroy) { // one does expect a hero position from the "begin" map.
    int16_t herox=ctx.herox*FMN_MM_PER_TILE+(FMN_MM_PER_TILE>>1);
    int16_t heroy=ctx.heroy*FMN_MM_PER_TILE+(FMN_MM_PER_TILE>>1);
    fmn_hero_set_position(herox,heroy);
  }
  
  struct fmn_sprite *hero=fmn_game_spawn_sprite(&fmn_sprite_type_hero,0,0,0,0);
}

/* Navigate to map.
 */
 
uint8_t fmn_game_navigate(const struct fmn_map_resource *map,uint8_t col,uint8_t row,uint8_t transition) {
  if (!map) return 0;
  
  // Clear up some state just in case.
  fmn_violin_end();
  
  struct fmn_game_cb_mapcmd_load_context ctx={0};
  int16_t herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  
  if (transition) {
    fmn_game_mode_play_render_pretransition(&fmn_play_transition_bits);
    fmn_play_transition.mode=transition;
    fmn_play_transition.p=0;
    fmn_play_transition.c=FMN_PLAY_TRANSITION_TIME;
    switch (transition) {
      case FMN_TRANSITION_SPOTLIGHT: {
          fmn_play_transition.outx=((int32_t)herox*FMN_TILESIZE)/FMN_MM_PER_TILE;
          fmn_play_transition.outy=((int32_t)heroy*FMN_TILESIZE)/FMN_MM_PER_TILE;
        } break;
    }
  } else {
    fmn_play_transition.c=0;
  }
  
  //TODO exit hooks
  fmn_spritev_clear();
  
  fmn_map_decode(&fmn_map,map);
  fmn_map_for_each_command(fmn_map.cmdv,fmn_game_cb_mapcmd_load,&ctx);
  
  // Update hero position.
  if (col||row) {
    herox=col*FMN_MM_PER_TILE+(FMN_MM_PER_TILE>>1);
    heroy=row*FMN_MM_PER_TILE+(FMN_MM_PER_TILE>>1);
  } else switch (transition) {
    case FMN_TRANSITION_PAN_LEFT: herox+=FMN_COLC*FMN_MM_PER_TILE; break;
    case FMN_TRANSITION_PAN_RIGHT: herox-=FMN_COLC*FMN_MM_PER_TILE; break;
    case FMN_TRANSITION_PAN_UP: heroy+=FMN_ROWC*FMN_MM_PER_TILE; break;
    case FMN_TRANSITION_PAN_DOWN: heroy-=FMN_ROWC*FMN_MM_PER_TILE; break;
    default: { // SPOTLIGHT,DISSOLVE2,etc: Expect position to be established by map.
        if (ctx.herox||ctx.heroy) {
          herox=ctx.herox*FMN_MM_PER_TILE+(FMN_MM_PER_TILE>>1);
          heroy=ctx.heroy*FMN_MM_PER_TILE+(FMN_MM_PER_TILE>>1);
        }
      }
  }
  if (transition==FMN_TRANSITION_SPOTLIGHT) {
    fmn_play_transition.inx=((int32_t)herox*FMN_TILESIZE)/FMN_MM_PER_TILE;
    fmn_play_transition.iny=((int32_t)heroy*FMN_TILESIZE)/FMN_MM_PER_TILE;
  }
  fmn_hero_set_position(herox,heroy);
  struct fmn_sprite *hero=fmn_game_spawn_sprite(&fmn_sprite_type_hero,0,0,0,0);
  if (hero) {
    hero->x=herox;
    hero->y=heroy;
  }
  
  return 1;
}

/* Input.
 */
 
void fmn_game_mode_play_input(uint8_t input,uint8_t pvinput) {
  fmn_hero_input(input,pvinput);
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
      return;
    }
  }
  
  if (fmn_play_light_time) {
    if (!--fmn_play_light_time) {
      fprintf(stderr,"TODO lights out [%s:%d]\n",__FILE__,__LINE__);
    }
  }
  
  fmn_hero_update();
  fmn_spritev_update();
  fmn_spritev_sort_partial();
  //TODO Weather? Other recurring update tasks?
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
  struct fmn_sprite *sprite=fmn_spritev;
  uint8_t i=fmn_spritec;
  for (;i-->0;sprite++) {
    if (!sprite->type) continue;
    int16_t dstx=((int32_t)sprite->x*FMN_TILESIZE)/FMN_MM_PER_TILE;
    int16_t dsty=((int32_t)sprite->y*FMN_TILESIZE)/FMN_MM_PER_TILE;
    if (sprite->type->render) {
      sprite->type->render(fb,sprite,dstx,dsty);
    } else if (sprite->tilesheet) {
      dstx-=FMN_TILESIZE>>1;
      dsty-=FMN_TILESIZE>>1;
      int16_t srcx=(sprite->tileid&15)*FMN_TILESIZE;
      int16_t srcy=(sprite->tileid>>4)*FMN_TILESIZE;
      fmn_image_blit(fb,dstx,dsty,sprite->tilesheet,srcx,srcy,FMN_TILESIZE,FMN_TILESIZE,sprite->xform);
    }
  }
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
  fmn_violin_render(fb);
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

/* Spawn sprite.
 */
 
struct fmn_sprite *fmn_game_spawn_sprite(
  const struct fmn_sprite_type *type,
  uint8_t col,uint8_t row,
  const uint8_t *argv,uint8_t argc
) {
  if (!type) return 0;
  struct fmn_sprite *sprite=fmn_sprite_alloc();
  if (!sprite) return 0;
  sprite->type=type;
  sprite->x=col*FMN_MM_PER_TILE+(FMN_MM_PER_TILE>>1);
  sprite->y=row*FMN_MM_PER_TILE+(FMN_MM_PER_TILE>>1);
  if (type->init) type->init(sprite,argv,argc);
  return sprite;
}

/* Soulballs.
 */
 
void fmn_game_create_soulballs(int16_t xmm,int16_t ymm) {
  fprintf(stderr,"TODO %s %d,%d\n",__func__,xmm,ymm);
}

/* Spells.
 */
 
//XXX
static void fmn_game_log_spell(const char *fnname,const uint8_t *v,uint8_t c) {
  fprintf(stderr,"%s:",fnname);
  for (;c-->0;v++) switch (*v) {
    case 0: fprintf(stderr," ."); break;
    case FMN_DIR_N: fprintf(stderr," N"); break;
    case FMN_DIR_S: fprintf(stderr," S"); break;
    case FMN_DIR_W: fprintf(stderr," W"); break;
    case FMN_DIR_E: fprintf(stderr," E"); break;
    default: fprintf(stderr," ?"); break;
  }
  fprintf(stderr,"\n");
}
 
uint8_t fmn_game_cast_spell(const uint8_t *v,uint8_t c) {
  fmn_game_log_spell(__func__,v,c);
  return 0;
}

uint8_t fmn_game_cast_song(const uint8_t *v,uint8_t c) {
  while (c&&!v[c-1]) c--;
  while (c&&!v[0]) { c--; v++; }
  if (!c) return 0;
  fmn_game_log_spell(__func__,v,c);
  return 0;
}

uint8_t fmn_game_pour_fluid(int16_t xmm,int16_t ymm,uint8_t content) {
  fprintf(stderr,"TODO %s (%d,%d) #%d\n",__func__,xmm,ymm,content);
  return 0;
}

void fmn_game_generate_light(uint16_t framec) {
  fprintf(stderr,"TODO %s %d\n",__func__,framec);
  fmn_play_light_time=framec;
}
