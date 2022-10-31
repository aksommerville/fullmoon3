/* fmn_machid_driver.c
 * Glue between fmn_machid and our 'intf' unit.
 * This only does Fullmoon-specific stuff; if you're copying to some new project you only need fmn_machid.[ch]
 */
 
#include "fmn_machid.h"
#include "opt/hw/fmn_hw.h"
#include <stdio.h>

static struct fmn_hw_input *fmn_machid_instance=0;

/* Object definition.
 */

struct fmn_hw_input_machid {
  struct fmn_hw_input hdr;
};

#define DRIVER ((struct fmn_hw_input_machid*)driver)

/* Cleanup.
 */

static void _machid_del(struct fmn_hw_input *driver) {
  fmn_machid_quit();
  if (driver==fmn_machid_instance) fmn_machid_instance=0;
}

/* Callbacks from machid proper.
 */

static int fmn_machid_cb_connect(int devid) {
  struct fmn_hw_input *driver=fmn_machid_instance;
  if (!driver) return 0;
  if (driver->delegate->connect) driver->delegate->connect(driver,devid);
  return 0;
}

static int fmn_machid_cb_disconnect(int devid) {
  struct fmn_hw_input *driver=fmn_machid_instance;
  if (!driver) return 0;
  if (driver->delegate->disconnect) driver->delegate->disconnect(driver,devid);
  return 0;
}

static int fmn_machid_cb_button(int devid,int btnid,int value) {
  //fprintf(stderr,"%s %d.%d=%d\n",__func__,devid,btnid,value);
  struct fmn_hw_input *driver=fmn_machid_instance;
  if (!driver) return 0;
  if (driver->delegate->button) driver->delegate->button(driver,devid,btnid,value);
  return 0;
}

/* Init.
 */

static int _machid_init(struct fmn_hw_input *driver) {
  if (fmn_machid_instance) {
    fprintf(stderr,"%s: Multiple instantiation\n",__func__);
    return -1;
  }
  fmn_machid_instance=driver;
  struct fmn_machid_delegate delegate={
    .connect=fmn_machid_cb_connect,
    .disconnect=fmn_machid_cb_disconnect,
    .button=fmn_machid_cb_button,
  };
  if (fmn_machid_init(&delegate)<0) return -1;
  return 0;
}

/* Update.
 */

static int _machid_update(struct fmn_hw_input *driver) {
  return fmn_machid_update(0.0);
}

/* Get device IDs.
 */

static const char *_machid_device_get_ids(int *vid,int *pid,struct fmn_hw_input *driver,int devid) {
  if (vid) *vid=fmn_machid_dev_get_vendor_id(devid);
  if (pid) *pid=fmn_machid_dev_get_product_id(devid);
  return fmn_machid_dev_get_product_name(devid);
}

/* Iterate device buttons.
 */

static int _machid_device_iterate(
  struct fmn_hw_input *driver,
  int devid,
  int (*cb)(struct fmn_hw_input *driver,int devid,int btnid,int hidusage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  int index=0,btnid,usage,lo,hi,value,err;
  for (;;index++) {
    if (fmn_machid_dev_get_button_info(&btnid,&usage,&lo,&hi,&value,devid,index)<0) break;
    if (err=cb(driver,devid,btnid,usage,lo,hi,value,userdata)) return err;
  }
  return 0;
}

/* Drop device.
 */

static int _machid_device_drop(struct fmn_hw_input *driver,int devid) {
  // machid proper doesn't support this, and i doubt we'll need it
  return -1;
}

/* Type definition.
 */

const struct fmn_hw_input_type fmn_hw_input_type_machid={
  .name="machid",
  .desc="USB-HID for MacOS",
  .objlen=sizeof(struct fmn_hw_input_machid),
  .del=_machid_del,
  .init=_machid_init,
  .update=_machid_update,
  .get_ids=_machid_device_get_ids,
  .enumerate=_machid_device_iterate,
  //.device_drop=_machid_device_drop,
};
