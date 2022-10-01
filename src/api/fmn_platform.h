/* fmn_platform.h
 * Facilities provided by optional units, for low-level operations.
 */
 
#ifndef FMN_PLATFORM_H
#define FMN_PLATFORM_H

#include <stdint.h>

struct fmn_image;

/* Setup and maintenance.
 ****************************************************************/

/* Client must call this early in setup().
 * Platform has some flexibility, whether to initialize things before setup, or during init.
 */
int8_t fmn_platform_init();

/* Request termination at the next convenient time.
 * Generally not synchronous.
 */
void fmn_platform_terminate(uint8_t status);

/* Poll drivers, etc.
 * Client must call this once during each loop().
 * Returns the current input state.
 */
uint8_t fmn_platform_update();

#define FMN_BUTTON_LEFT   0x01
#define FMN_BUTTON_RIGHT  0x02
#define FMN_BUTTON_UP     0x04
#define FMN_BUTTON_DOWN   0x08
#define FMN_BUTTON_A      0x10
#define FMN_BUTTON_B      0x20

/* Video.
 * This interface could be implemented in software or with GX. Client shouldn't care.
 *****************************************************************/

/* Assign a raw image, eg from resource manager, to a renderable texture.
 * Platform must copy the image. Caller is free to modify or delete it after upload.
 * (Immutable immortal images may of course be retained directly).
 */
void fmn_platform_video_upload_image(
  uint16_t imageid,
  struct fmn_image *image
);
 
/* You should begin/end once per loop(), and all rendering calls must be between them.
 */
void fmn_platform_video_begin();
void fmn_platform_video_end();

void fmn_platform_video_fill_rect(
  int16_t x,int16_t y,int16_t w,int16_t h,
  uint32_t rgba
);

void fmn_platform_video_blit(
  int16_t dstx,int16_t dsty,
  uint16_t srcimageid,
  int16_t srcx,int16_t srcy,
  int16_t w,int16_t h,
  uint8_t xform
);

void fmn_platform_video_blit_tile(
  int16_t dstx,int16_t dsty,
  uint16_t srcimageid,
  uint8_t tileid,
  uint8_t xform
);

/* Audio.
 *************************************************************/

/* Apply instrument and sound effects definitions from a resource.
 */
void fmn_platform_audio_configure(const void *v,uint16_t c);

/* Begin playing a song, if it isn't already.
 * Play (0,0) for none.
 * *** We will read off (v) directly, caller must arrange to keep it alive. ***
 */
void fmn_platform_audio_play_song(const void *v,uint16_t c);

/* Play a fire-and-forget note with no channel context.
 * This is probably what you want, for sound effects initiated by the game.
 */
void fmn_platform_audio_note(uint8_t programid,uint8_t noteid,uint8_t velocity,uint16_t duration_ms);

/* Stop all output cold, or release all notes gently.
 * These do not stop the song from playing, but they do interrupt the song's held notes.
 */
void fmn_platform_audio_silence();
void fmn_platform_audio_release_all();

#endif
