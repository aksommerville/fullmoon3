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
};

/* Encoded maps are (FMN_COLC*FMN_ROWC==96) bytes of tile data, followed by a terminated command list.
 * There is a separate TOC of object references, which the serial commands refer to.
 * These are separate from the serial data because Arduino is a little weird about object lengths in progmem.
 * "decode" just copies the cells and yoinks a reference to the commands.
 */
void fmn_map_decode(struct fmn_map *dst,const void *src,const void **refv);

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
#define FMN_MAP_CMD_SPRITE       0x80
#define FMN_MAP_CMD_EVENTV       0xe0

int8_t fmn_map_for_each_command(
  const uint8_t *cmdv,
  int8_t (*cb)(uint8_t cmd,const uint8_t *argv,uint8_t argc,void *userdata),
  void *userdata
);

#endif
