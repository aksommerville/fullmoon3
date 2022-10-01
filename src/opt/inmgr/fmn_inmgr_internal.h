#ifndef FMN_INMGR_INTERNAL_H
#define FMN_INMGR_INTERNAL_H

#include "fmn_inmgr.h"
#include "api/fmn_platform.h"
#include "opt/hw/fmn_hw.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

struct fmn_inmgr {
  struct fmn_inmgr_delegate delegate;
  uint8_t state;
  struct fmn_inmgr_device {
    int devid;
    uint8_t state;
    struct fmn_inmgr_device_button {
      int srcbtnid;
      int dstbtnid;
      int srcvalue;
      int dstvalue;
      int srclo,srchi;
    } *buttonv;
    int buttonc,buttona;
  } *devicev;
  int devicec,devicea;
};

void fmn_inmgr_device_cleanup(struct fmn_inmgr_device *device);

void fmn_inmgr_clear_state(struct fmn_inmgr *inmgr);

int fmn_inmgr_devicev_search(const struct fmn_inmgr *inmgr,int devid);
struct fmn_inmgr_device *fmn_inmgr_devicev_insert(struct fmn_inmgr *inmgr,int p,int devid);

// Duplicate (srcbtnid) are permitted, and records not sorted beyond that. Search will return the first.
int fmn_inmgr_device_buttonv_search(const struct fmn_inmgr_device *device,int srcbtnid);
struct fmn_inmgr_device_button *fmn_inmgr_device_buttonv_insert(struct fmn_inmgr_device *device,int p,int srcbtnid);

#endif
