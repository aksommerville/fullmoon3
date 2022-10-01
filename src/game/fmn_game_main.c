#include "api/fmn_platform.h"
#include "api/fmn_client.h"

void setup() {
  if (fmn_platform_init()<0) return;
}

void loop() {
  uint8_t input=fmn_platform_update();
  fmn_platform_video_begin();
  fmn_platform_video_fill_rect(0,0,10000,10000,0x808080ff);
  fmn_platform_video_fill_rect(1,1,1,1,0x800000ff);
  fmn_platform_video_fill_rect(3,1,1,1,0x800000ff);
  fmn_platform_video_fill_rect(2,2,1,1,0x800000ff);
  fmn_platform_video_fill_rect(1,3,1,1,0x800000ff);
  fmn_platform_video_fill_rect(3,3,1,1,0x800000ff);
  fmn_platform_video_end();
}
