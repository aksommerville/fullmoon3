/* fmn_image.h
 * Data type for images both live and at rest.
 * We may contain raw pixels or some high-level encoding like PNG.
 * There may be a single image format established at build time.
 */
 
#ifndef FMN_IMAGE_H
#define FMN_IMAGE_H

#include <stdint.h>

/* Image formats and capabilities, selected at build time.
 * To reduce the burden on the little platforms, we may choose to support just one image format.
 *********************************************************************/

#define FMN_IMAGE_FMT_ENCODED      0x01 /* Heavy encoding, typically PNG. Infer from content. (stride) is total length in bytes. */
#define FMN_IMAGE_FMT_RGBA         0x02 /* Big-endian 32-bit RGBA. ck=0 */
#define FMN_IMAGE_FMT_Y8           0x03 /* ck=0x81 */
#define FMN_IMAGE_FMT_Y2           0x04 /* Big-endian 2-bit gray. ck=2 */

#define FMN_FOR_EACH_IMAGE_FMT \
  _(ENCODED) \
  _(RGBA) \
  _(Y8) \
  _(Y2)
  
/* FMN_IMAGE_CAPS is bitfields, 1<<FMN_IMAGE_FMT_* for all supported formats.
 * Having this known at compile time lets us eliminate code for unsupported formats, a big win for Tiny.
 */
#ifndef FMN_IMAGE_CAPS
  #define FMN_IMAGE_CAPS 0xfe
#endif

#define FMN_IMAGE_FMT_SUPPORTED(tag) (FMN_IMAGE_CAPS&(1<<FMN_IMAGE_FMT_##tag))
  
/* Image object.
 **********************************************************************/

#define FMN_IMAGE_SIZE_LIMIT 1024

#define FMN_IMAGE_FLAG_TRANSPARENT 0x01 /* Uses a color key (value depends on fmt) */
#define FMN_IMAGE_FLAG_WRITEABLE   0x02

#define FMN_XFORM_XREV 1
#define FMN_XFORM_YREV 2
// We are not supporting a SWAP bit; it's complicated and I don't think we're going to need rotation.

struct fmn_image {
  void *v;
  int16_t w,h;
  int16_t stride; // Usually bytes row-to-row, but may depend on (fmt).
  uint8_t fmt;
  uint8_t flags;
  uint8_t refc; // 0=immortal
};

void fmn_image_del(struct fmn_image *image);
int8_t fmn_image_ref(struct fmn_image *image);

struct fmn_image *fmn_image_new_alloc(uint8_t fmt,int16_t w,int16_t h);

int16_t fmn_image_measure(uint8_t fmt,int16_t w,int16_t h);
int16_t fmn_image_minimum_stride(uint8_t fmt,int16_t w,int16_t h);

/* Rendering.
 ***********************************************************************/
 
uint32_t fmn_pixel_from_rgba(uint8_t fmt,uint8_t r,uint8_t g,uint8_t b,uint8_t a);

void fmn_image_fill_rect(struct fmn_image *image,int16_t x,int16_t y,int16_t w,int16_t h,uint32_t pixel);

/* Blitting may only be done between images of like format.
 */
void fmn_image_blit(
  struct fmn_image *dst,int16_t dstx,int16_t dsty,
  struct fmn_image *src,int16_t srcx,int16_t srcy,
  int16_t w,int16_t h,
  uint8_t xform
);

#endif
