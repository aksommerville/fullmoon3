/* fmn_map.h
 */
 
#ifndef FMN_MAP_H
#define FMN_MAP_H

#include <stdint.h>
#include "api/fmn_common.h"

struct fmn_map {
  uint8_t v[FMN_COLC*FMN_ROWC]; // mutable; copied at load
  const uint8_t *cmdv; // must be terminated
  const void **refv;
// Decoding does not populate these. Caller should:
  const struct fmn_image *tilesheet;
  const struct fmn_map_resource *neighborw,*neighbore,*neighborn,*neighbors;
  uint8_t has_hero_cell_features; // nonzero if hero should check the map on cell changes (eg doors present)
};

/* As stored in program image.
 * (serial) is terminated by FMN_MAP_CMD_EOF after (FMN_COLC*FMN_ROWC) bytes of cell data.
 * (refv) has no declared length; consumer may assume that all references are in range.
 */
struct fmn_map_resource {
  const uint8_t *serial;
  const void **refv;
};

/* Copy cells and record pointers to commands and references.
 * Note that nothing beyond (v,cmdv,refv) is populated during decode.
 */
void fmn_map_decode(struct fmn_map *dst,const struct fmn_map_resource *res);

/* The high 3 bits of a command disclose its argument length:
 *   000 0
 *   001 1
 *   010 2
 *   011 4
 *   100 6
 *   101 8
 *   110 12
 *   111 length in first arg byte (total 2..257)
 */
#define FMN_MAP_CMD_EOF          0x00
#define FMN_MAP_CMD_TILESHEET    0x20
#define FMN_MAP_CMD_SONG         0x21
#define FMN_MAP_CMD_HERO         0x22
#define FMN_MAP_CMD_NEIGHBORW    0x23
#define FMN_MAP_CMD_NEIGHBORE    0x24
#define FMN_MAP_CMD_NEIGHBORN    0x25
#define FMN_MAP_CMD_NEIGHBORS    0x26
#define FMN_MAP_CMD_HOME         0x27
#define FMN_MAP_CMD_EVENT1       0x60
#define FMN_MAP_CMD_CELLIF       0x61
#define FMN_MAP_CMD_DOOR         0x62
#define FMN_MAP_CMD_SPRITE       0x80
#define FMN_MAP_CMD_EVENTV       0xe0

int8_t fmn_map_for_each_command(
  const uint8_t *cmdv,
  int8_t (*cb)(uint8_t cmd,const uint8_t *argv,uint8_t argc,void *userdata),
  void *userdata
);

/* Nonzero if we loaded the new map.
 * (col,row)=(0,0) is technically valid, but unlikely in real life, so we treat that as "unspecified".
 * Use (0,0) to select a hero position automatically based on (transition), map commands, or current position.
 */
uint8_t fmn_game_navigate(const struct fmn_map_resource *map,uint8_t col,uint8_t row,uint8_t transition);

#endif
