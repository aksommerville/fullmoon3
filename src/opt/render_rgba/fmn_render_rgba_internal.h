#ifndef FMN_RENDER_RGBA_INTERNAL_H
#define FMN_RENDER_RGBA_INTERNAL_H

#include "opt/hw/fmn_hw.h"
#include "api/fmn_common.h"
#include "game/image/fmn_image.h"
#include <stdio.h>
#include <endian.h>

struct fmn_hw_render_rgba {
  struct fmn_hw_render hdr;
  struct fmn_image *fb;
};

#define RENDER ((struct fmn_hw_render_rgba*)render)

#endif
