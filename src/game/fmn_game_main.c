#include "api/fmn_platform.h"
#include "api/fmn_client.h"
#include "game/image/fmn_image.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define WORLDW 96
#define WORLDH 64

extern const struct fmn_image fmnr_image_hero;
extern const struct fmn_image fmnr_image_outdoors;
extern const uint8_t fmnr_song_tangled_vine[];
extern const uint16_t fmnr_song_tangled_vine_length;
extern const uint8_t fmnr_song_seven_circles[];
extern const uint16_t fmnr_song_seven_circles_length;

static uint8_t image_rgba_pixels[]={
#define _ 0,0,0,0,
#define K 0,0,0,255,
#define W 255,255,255,255,
#define R 255,0,0,255,
  _ _ _ K K _ _ _
  _ _ K R W K _ _
  _ K R R W W K _
  K R R R W W W K
  K W W W W W W K
  _ K W W W W K _
  _ _ K W W K _ _
  _ _ _ K K _ _ _
#undef _
#undef K
#undef W
#undef R
};
static struct fmn_image image_rgba={
  .fmt=FMN_IMAGE_FMT_RGBA,
  .w=8,
  .h=8,
  .stride=8*4,
  .v=image_rgba_pixels,
  .flags=FMN_IMAGE_FLAG_TRANSPARENT,
};

static uint8_t image_y8_pixels[]={
#define _ 0x81,
#define K 0x00,
#define W 0xff,
#define D 0x40,
#define L 0xc0,
  _ _ _ K K _ _ _
  _ _ K D L K _ _
  _ K D D L L K _
  K D D D L L L K
  K L L L W W W K
  _ K L L W W K _
  _ _ K L W K _ _
  _ _ _ K K _ _ _
#undef _
#undef K
#undef W
#undef D
#undef L
};
static struct fmn_image image_y8={
  .fmt=FMN_IMAGE_FMT_Y8,
  .w=8,
  .h=8,
  .stride=8,
  .v=image_y8_pixels,
  .flags=FMN_IMAGE_FLAG_TRANSPARENT,
};

static uint8_t image_y2_pixels[]={
#define P(a,b,c,d) (a<<6)|(b<<4)|(c<<2)|d,
#define _ 2
#define K 0
#define D 1
#define W 3
  P(_,_,_,K) P(K,_,_,_)
  P(_,_,K,D) P(W,K,_,_)
  P(_,K,D,D) P(W,W,K,_)
  P(K,D,D,D) P(W,W,W,K)
  P(K,W,W,W) P(W,W,W,K)
  P(_,K,W,W) P(W,W,K,_)
  P(_,_,K,W) P(W,K,_,_)
  P(_,_,_,K) P(K,_,_,_)
#undef P
#undef _
#undef K
#undef D
#undef W
};
static struct fmn_image image_y2={
  .fmt=FMN_IMAGE_FMT_Y2,
  .w=8,
  .h=8,
  .stride=2,
  .v=image_y2_pixels,
  .flags=FMN_IMAGE_FLAG_TRANSPARENT,
};

#define SPRITEC 100
static struct sprite {
  int16_t x,y;
  int16_t dx,dy;
  uint8_t xform;
  uint8_t tileid;
  const struct fmn_image *image;
} spritev[SPRITEC];

static const uint8_t my_fake_song[]={
  100, // tempo, ms/tick
  4,0,4,
  
  /*
0000 0000                      : EOF
0ttt tttt                      : Delay (t) ticks.
100v vvvv  nnnn nndd  dddd cccc: Note: (v)elocity, (n)oteid, (d)uration, (c)hannel.
1010 cccc  kkkk kkkk  vvvv vvvv: Config: (c)hannel, (k)ey, (v)alue.
1011 cccc  0nnn nnnn  0vvv vvvv: Note On: (c)hannel, (n)oteid, (v)elocity.
1100 cccc  0nnn nnnn           : Note Off: (c)hannel, (n)oteid.
  */
  0x90,0x10,0x10,
  0x02,
  0x90,0x1c,0x10,
  0x02,
  0x90,0x2c,0x10,
  0x02,
  0x90,0x40,0x10,
  0x02,
  0x00,
};

void setup() {
  srand(time(0));
  
  if (fmn_platform_init()<0) return;
  
  uint8_t fbfmt=fmn_platform_video_get_format();
  struct sprite *sprite=spritev;
  int i=SPRITEC;
  for (;i-->0;sprite++) {
    sprite->x=rand()%WORLDW;
    sprite->y=rand()%WORLDH;
    sprite->dx=rand()%3-1;
    sprite->dy=rand()%3-1;
    sprite->xform=rand()&3;
    switch (fbfmt) {
      case FMN_IMAGE_FMT_RGBA: sprite->image=&image_rgba; break;
      case FMN_IMAGE_FMT_Y8: sprite->image=&image_y8; break;
      case FMN_IMAGE_FMT_Y2: sprite->image=&image_y2; break;
    }
    sprite->image=&fmnr_image_hero;
    switch (rand()%3) {
      case 0: sprite->tileid=0x11; break;
      case 1: sprite->tileid=0x13; break;
      case 2: sprite->tileid=0x21; break;
    }
  }
  
  fmn_platform_audio_play_song(fmnr_song_tangled_vine,fmnr_song_tangled_vine_length);
  //fmn_platform_audio_play_song(fmnr_song_seven_circles,fmnr_song_seven_circles_length);
  //fmn_platform_audio_play_song(my_fake_song,sizeof(my_fake_song));
}

static int16_t x=1,y=1;
static uint8_t pvinput=0;

void loop() {
  uint8_t input=fmn_platform_update();
  
  switch (input&(FMN_BUTTON_LEFT|FMN_BUTTON_RIGHT)) {
    case FMN_BUTTON_LEFT: if (x>0) x--; break;
    case FMN_BUTTON_RIGHT: if (x<93) x++; break;
  }
  switch (input&(FMN_BUTTON_UP|FMN_BUTTON_DOWN)) {
    case FMN_BUTTON_UP: if (y>0) y--; break;
    case FMN_BUTTON_DOWN: if (y<61) y++; break;
  }
  
  if (input!=pvinput) {
    if ((input&FMN_BUTTON_A)&&!(pvinput&FMN_BUTTON_A)) fmn_platform_audio_note(0,0x40,0x40,0);
    if ((input&FMN_BUTTON_B)&&!(pvinput&FMN_BUTTON_B)) fmn_platform_audio_note(0,0x47,0x40,100);
    pvinput=input;
  }
  
  struct sprite *sprite=spritev;
  int i=SPRITEC;
  int tilesize=8;
  if (!(input&FMN_BUTTON_A)) {
  for (;i-->0;sprite++) {
    sprite->x+=sprite->dx;
    sprite->y+=sprite->dy;
    if (sprite->dx<0) {
      if (sprite->x+tilesize<=0) sprite->x+=WORLDW+tilesize;
    } else {
      if (sprite->x>=WORLDW) sprite->x-=WORLDW+tilesize;
    }
    if (sprite->dy<0) {
      if (sprite->y+tilesize<=0) sprite->y+=WORLDH+tilesize;
    } else {
      if (sprite->y>=WORLDH) sprite->y-=WORLDH+tilesize;
    }
  }
  }
  
  struct fmn_image *fb=fmn_platform_video_begin();
  if (fb) {
    //fmn_image_fill_rect(fb,0,0,fb->w,fb->h,fmn_pixel_from_rgba(fb->fmt,0xff,0x80,0x00,0xff));
    fmn_image_blit(fb,0,0,&fmnr_image_outdoors,0,0,fb->w,fb->h,0);
  
    for (sprite=spritev,i=SPRITEC;i-->0;sprite++) {
      //fmn_image_blit(fb,sprite->x,sprite->y,sprite->image,0,0,sprite->image->w,sprite->image->h,sprite->xform);
      fmn_image_blit_tile(fb,sprite->x+4,sprite->y+4,sprite->image,sprite->tileid,sprite->xform);
    }
  
    // A little white-in-blue box you can move with the dpad.
    fmn_image_fill_rect(fb,x,y,3,3,fmn_pixel_from_rgba(fb->fmt,0x00,0x00,0x80,0xff));
    fmn_image_fill_rect(fb,x+1,y+1,1,1,fmn_pixel_from_rgba(fb->fmt,0xff,0xff,0xff,0xff));
    
    fmn_platform_video_end(fb);
  }
}
