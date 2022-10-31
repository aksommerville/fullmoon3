/* fmn_machid.h
 * Standalone C API for HID in MacOS.
 * You must call all fmn_machid functions from the same thread.
 * It does not need to be the main thread. (TODO confirm this)
 * Link: -framework IOKit
 */

#ifndef FMN_MACHID_H
#define FMN_MACHID_H

/* All delegate functions are optional.
 * All except test_device can return <0 for error.
 * We will return the same error code through fmn_machid_update().
 * There may be additional callbacks before an error gets reported; it doesn't abort all processing.
 */
struct fmn_machid_delegate {

  /* Return nonzero to accept this device.
   * No errors.
   * Unset for the default behavior (Joystick, Game Pad, and Multi-Axis Controller).
   * This is called before full connection, so we don't have complete device properties yet.
   */
  int (*test_device)(void *hiddevice,int vid,int pid,int usagepage,int usage);

  /* Notify connection or disconnection of a device.
   * This is the first and last time you will see this devid.
   * If it appears again, it's a new device.
   */
  int (*connect)(int devid);
  int (*disconnect)(int devid);

  /* Notify of activity on a device.
   */
  int (*button)(int devid,int btnid,int value);

};

/* Main public API.
 */

int fmn_machid_init(const struct fmn_machid_delegate *delegate);

void fmn_machid_quit();

/* Trigger our private run loop mode and return any errors gathered during it.
 */
int fmn_machid_update(double timeout);

/* Access to installed devices.
 */

int fmn_machid_count_devices();
int fmn_machid_devid_for_index(int index);
int fmn_machid_index_for_devid(int devid);

void *fmn_machid_dev_get_IOHIDDeviceRef(int devid);
int fmn_machid_dev_get_vendor_id(int devid);
int fmn_machid_dev_get_product_id(int devid);
int fmn_machid_dev_get_usage_page(int devid);
int fmn_machid_dev_get_usage(int devid);
const char *fmn_machid_dev_get_manufacturer_name(int devid);
const char *fmn_machid_dev_get_product_name(int devid);
const char *fmn_machid_dev_get_serial_number(int devid);

int fmn_machid_dev_count_buttons(int devid);
int fmn_machid_dev_get_button_info(int *btnid,int *usage,int *lo,int *hi,int *value,int devid,int index);

#endif
