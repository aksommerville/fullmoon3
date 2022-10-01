/* fmn_image.h
 * Data type for images both live and at rest.
 * We may contain raw pixels or some high-level encoding like PNG.
 * There may be a single image format established at build time.
 */
 
#ifndef FMN_IMAGE_H
#define FMN_IMAGE_H

#include <stdint.h>

#define FMN_IMAGE_SIZE_LIMIT 1024

#define FMN_IMAGE_FMT_ENCODED      0x00 /* Heavy encoding, typically PNG. Infer from content. (stride) is total length in bytes. */
#define FMN_IMAGE_FMT_RGBA         0x01 /* Big-endian 32-bit RGBA. ck=0 */
#define FMN_IMAGE_FMT_Y8           0x02 /* ck=0x81 */
#define FMN_IMAGE_FMT_Y2           0x03 /* Big-endian 2-bit gray. ck=2 */

#define FMN_IMAGE_FLAG_TRANSPARENT 0x01 /* Uses a color key (value depends on fmt) */
#define FMN_IMAGE_FLAG_WRITEABLE   0x02

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

#endif
