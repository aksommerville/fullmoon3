#ifndef FMN_DATA_H
#define FMN_DATA_H

#include <stdint.h>

struct fmn_image;
struct fmn_map_resource;

extern struct fmn_image fmnr_image_hero;
extern struct fmn_image fmnr_image_outdoors;
extern struct fmn_image fmnr_image_uibits;
extern struct fmn_image fmnr_image_sprites1;

//TODO lengths are not necessary; songs must be self-terminated.
extern const uint8_t fmnr_song_tangled_vine[]; extern const uint16_t fmnr_song_tangled_vine_length;
extern const uint8_t fmnr_song_seven_circles[]; extern const uint16_t fmnr_song_seven_circles_length;

//TODO We don't need to declare every map. Just the ones accessible programmatically, eg game start.
extern const struct fmn_map_resource fmnr_map_begin;
extern const struct fmn_map_resource fmnr_map_test_east;
extern const struct fmn_map_resource fmnr_map_test_south;
extern const struct fmn_map_resource fmnr_map_test_se;

#endif
