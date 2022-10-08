#ifndef FMN_DATA_H
#define FMN_DATA_H

#include <stdint.h>

struct fmn_image;

extern const struct fmn_image fmnr_image_hero;
extern const struct fmn_image fmnr_image_outdoors;
extern const struct fmn_image fmnr_image_uibits;

extern const uint8_t fmnr_song_tangled_vine[]; extern const uint16_t fmnr_song_tangled_vine_length;
extern const uint8_t fmnr_song_seven_circles[]; extern const uint16_t fmnr_song_seven_circles_length;

extern const uint8_t fmnr_map_begin[]; extern const void *fmnr_map_begin_refv[];

#endif
