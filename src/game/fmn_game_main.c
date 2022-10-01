#include "api/fmn_platform.h"
#include "api/fmn_client.h"
#include "game/image/fmn_image.h"
#include <string.h>

void setup() {
  if (fmn_platform_init()<0) return;
}

static int16_t x=1,y=1;

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
  
  struct fmn_image *fb=fmn_platform_video_begin();
  if (fb) {
    memset(fb->v,0x80,fb->w*fb->h*4);
    ((uint32_t*)fb->v)[(y+0)*fb->w+x+0]=(input&FMN_BUTTON_A)?0xffffffff:(input&FMN_BUTTON_B)?0xff000000:0xffff0000;
    ((uint32_t*)fb->v)[(y+0)*fb->w+x+2]=(input&FMN_BUTTON_A)?0xffffffff:(input&FMN_BUTTON_B)?0xff000000:0xff00ff00;
    ((uint32_t*)fb->v)[(y+1)*fb->w+x+1]=(input&FMN_BUTTON_A)?0xffffffff:(input&FMN_BUTTON_B)?0xff000000:0xff0000ff;
    ((uint32_t*)fb->v)[(y+2)*fb->w+x+0]=(input&FMN_BUTTON_A)?0xffffffff:(input&FMN_BUTTON_B)?0xff000000:0xffffff00;
    ((uint32_t*)fb->v)[(y+2)*fb->w+x+2]=(input&FMN_BUTTON_A)?0xffffffff:(input&FMN_BUTTON_B)?0xff000000:0xff00ffff;
    fmn_platform_video_end(fb);
  }
}
