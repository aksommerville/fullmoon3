/* tool_platform_stubs.c
 * Tools don't have a valid platform, and these shouldn't ever get called.
 * But they must be implemented in order to link against game code (eg fmn_image).
 */
 
#include "api/fmn_platform.h"
#include "game/image/fmn_image.h"

int8_t fmn_platform_init() { return 0; }
void fmn_platform_terminate(uint8_t status) {}
uint8_t fmn_platform_update() { return 0; }
uint8_t fmn_platform_video_get_format() { return FMN_IMAGE_FMT_RGBA; }
struct fmn_image *fmn_platform_video_begin() { return 0; }
void fmn_platform_video_end(struct fmn_image *fb) {}
void fmn_platform_audio_configure(const void *v,uint16_t c) {}
void fmn_platform_audio_play_song(const void *v,uint16_t c) {}
void fmn_platform_audio_note(uint8_t programid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms) {}
void fmn_platform_audio_silence() {}
void fmn_platform_audio_release_all() {}
