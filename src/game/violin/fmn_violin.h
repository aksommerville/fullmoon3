/* fmn_violin.h
 * Manages interactive song input.
 */
 
#ifndef FMN_VIOLIN_H
#define FMN_VIOLIN_H

#include <stdio.h>

struct fmn_image;

void fmn_violin_begin();
void fmn_violin_end();

// Call each frame while violin is running, with the dpad's direction.
void fmn_violin_update(uint8_t dir);

void fmn_violin_render(struct fmn_image *fb);

#endif
