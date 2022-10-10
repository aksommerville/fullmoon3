/* fmn_common.h
 */
 
#ifndef FMN_COMMON_H
#define FMN_COMMON_H

#define FMN_TILESIZE 8
#define FMN_MM_PER_TILE 1024

#define FMN_COLC 12
#define FMN_ROWC 8

// Cardinal and diagonal directions, also suitable as neighbor masks.
#define FMN_DIR_NW   0x80
#define FMN_DIR_N    0x40
#define FMN_DIR_NE   0x20
#define FMN_DIR_W    0x10
#define FMN_DIR_CTR  0x00
#define FMN_DIR_E    0x08
#define FMN_DIR_SW   0x04
#define FMN_DIR_S    0x02
#define FMN_DIR_SE   0x01

#endif
