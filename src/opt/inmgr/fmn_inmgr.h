/* fmn_inmgr.h
 * Generic input manager.
 * Every target that uses unknown input devices should use this for mapping.
 */
 
#ifndef FMN_INMGR_H
#define FMN_INMGR_H

#include <stdint.h>

struct fmn_inmgr;
struct fmn_hw_input;

// Button IDs 256 and above are stateless actions.
#define FMN_INMGR_ACTION_QUIT         0x100
#define FMN_INMGR_ACTION_FULLSCREEN   0x101

#define FMN_INMGR_FOR_EACH_ACTION \
  _(QUIT) \
  _(FULLSCREEN)

struct fmn_inmgr_delegate {
  void *userdata;
  void (*state)(struct fmn_inmgr *inmgr,uint8_t btnid,int value,uint8_t state);
  void (*action)(struct fmn_inmgr *inmgr,int btnid);
};

//TODO inmgr config from file

void fmn_inmgr_del(struct fmn_inmgr *inmgr);

struct fmn_inmgr *fmn_inmgr_new(const struct fmn_inmgr_delegate *delegate);

void *fmn_inmgr_get_userdata(const struct fmn_inmgr *inmgr);

/* These correspond to the 4 hw_input callbacks.
 * Pass a null (input) at (connect) to configure the system keyboard.
 * We do like to see (premapped), to ensure our state remains consistent with your final output.
 */
int fmn_inmgr_connect(struct fmn_inmgr *inmgr,struct fmn_hw_input *input,int devid);
int fmn_inmgr_disconnect(struct fmn_inmgr *inmgr,int devid);
int fmn_inmgr_button(struct fmn_inmgr *inmgr,int devid,int btnid,int value);
int fmn_inmgr_premapped(struct fmn_inmgr *inmgr,int devid,uint8_t btnid,int value);

#endif
