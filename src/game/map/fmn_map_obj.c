#include "fmn_map.h"
#include <string.h>

/* Decode map.
 */
 
void fmn_map_decode(struct fmn_map *dst,const void *src,const void **refv) {
  memcpy(dst->v,src,FMN_COLC*FMN_ROWC);
  dst->cmdv=(uint8_t*)src+FMN_COLC*FMN_ROWC;
  dst->refv=refv;
}

/* Iterate commands.
 */

int8_t fmn_map_for_each_command(
  const uint8_t *cmdv,
  int8_t (*cb)(uint8_t cmd,const uint8_t *argv,uint8_t argc,void *userdata),
  void *userdata
) {
  if (!cmdv) return 0;
  int8_t err;
  while (*cmdv) {
    uint8_t cmd=*cmdv++;
    uint8_t argc=0;
    switch (cmd&0xe0) {
      case 0x00: break;
      case 0x20: argc=1; break;
      case 0x40: argc=2; break;
      case 0x60: argc=4; break;
      case 0x80: argc=6; break;
      case 0xa0: argc=8; break;
      case 0xc0: argc=12; break;
      case 0xe0: argc=*cmdv++; break;
    }
    if (err=cb(cmd,cmdv,argc,userdata)) return err;
    cmdv+=argc;
  }
  return 0;
}
