#include "fmn_machid.h"
#include "opt/hw/fmn_hw.h" /* only needed for fmn_hw_devid_next(); ok to remove if you've copied this code */
#include <stdio.h>
#include <IOKit/hid/IOHIDLib.h>

/* Private definitions.
 */

#define AKMACHID_RUNLOOP_MODE CFSTR("com.aksommerville.fmn_machid")

struct fmn_machid_btn {
  int btnid;
  int usage;
  int value;
  int lo,hi;
  int btnid_aux; // If nonzero, this is a hat switch to convert from polar to cartesian.
};

struct fmn_machid_dev {
  IOHIDDeviceRef obj; // weak
  int devid;
  int vid,pid;
  char *mfrname,*prodname,*serial;
  int usagepage,usage;
  struct fmn_machid_btn *btnv; int btnc,btna;
};

/* Globals.
 */

static struct {
  IOHIDManagerRef hidmgr;
  struct fmn_machid_dev *devv; // Sorted by (obj)
  int devc,deva;
  struct fmn_machid_delegate delegate; // All callbacks are set, possibly with defaults.
  int error; // Sticky error from callbacks, reported at end of update.
} fmn_machid={0};

/* Get integer field from device.
 */

static int dev_get_int(IOHIDDeviceRef dev,CFStringRef k) {
  CFNumberRef vobj=IOHIDDeviceGetProperty(dev,k);
  if (!vobj) return 0;
  if (CFGetTypeID(vobj)!=CFNumberGetTypeID()) return 0;
  int v=0;
  CFNumberGetValue(vobj,kCFNumberIntType,&v);
  return v;
}

/* Cleanup device record.
 */

static void fmn_machid_dev_cleanup(struct fmn_machid_dev *dev) {
  if (!dev) return;
  if (dev->btnv) free(dev->btnv);
  if (dev->mfrname) free(dev->mfrname);
  if (dev->prodname) free(dev->prodname);
  if (dev->serial) free(dev->serial);
  memset(dev,0,sizeof(struct fmn_machid_dev));
}

/* Search global device list.
 */

static int fmn_machid_dev_search_devid(int devid) {
  int i; for (i=0;i<fmn_machid.devc;i++) if (fmn_machid.devv[i].devid==devid) return i;
  return -fmn_machid.devc-1;
}

static int fmn_machid_dev_search_obj(IOHIDDeviceRef obj) {
  int lo=0,hi=fmn_machid.devc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (obj<fmn_machid.devv[ck].obj) hi=ck;
    else if (obj>fmn_machid.devv[ck].obj) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int fmn_machid_devid_unused() {
  // For Full Moon:
  return fmn_hw_devid_next();
#if 0 // The original standalone algorithm:
  if (fmn_machid.devc<1) return 1;
  
  /* Most cases: return one more than the highest used ID. */
  int i,top=0;
  for (i=0;i<fmn_machid.devc;i++) {
    if (fmn_machid.devv[i].devid>top) top=fmn_machid.devv[i].devid;
  }
  if (top<INT_MAX) return top+1;

  /* If we've reached ID INT_MAX, there must be a gap somewhere. Find it. */
  int devid=1;
  for (i=0;i<fmn_machid.devc;i++) if (fmn_machid.devv[i].devid==devid) { devid++; i=-1; }
  return devid;
#endif
}

/* Insert device to global list.
 */

static int fmn_machid_dev_insert_validate(int p,IOHIDDeviceRef obj,int devid) {
  if ((p<0)||(p>fmn_machid.devc)) return -1;
  if (!obj||(CFGetTypeID(obj)!=IOHIDDeviceGetTypeID())) return -1;
  if ((devid<1)||(devid>0xffff)) return -1;
  if (p&&(obj<=fmn_machid.devv[p-1].obj)) return -1;
  if ((p<fmn_machid.devc)&&(obj>=fmn_machid.devv[p].obj)) return -1;
  int i; for (i=0;i<fmn_machid.devc;i++) if (fmn_machid.devv[i].devid==devid) return -1;
  return 0;
}

static int fmn_machid_devv_require() {
  if (fmn_machid.devc<fmn_machid.deva) return 0;
  int na=fmn_machid.deva+8;
  if (na>INT_MAX/sizeof(struct fmn_machid_dev)) return -1;
  void *nv=realloc(fmn_machid.devv,sizeof(struct fmn_machid_dev)*na);
  if (!nv) return -1;
  fmn_machid.devv=nv;
  fmn_machid.deva=na;
  return 0;
}

static int fmn_machid_dev_insert(int p,IOHIDDeviceRef obj,int devid) {

  if (fmn_machid_dev_insert_validate(p,obj,devid)<0) return -1;
  if (fmn_machid_devv_require()<0) return -1;
  
  struct fmn_machid_dev *dev=fmn_machid.devv+p;
  memmove(dev+1,dev,sizeof(struct fmn_machid_dev)*(fmn_machid.devc-p));
  fmn_machid.devc++;
  memset(dev,0,sizeof(struct fmn_machid_dev));
  dev->obj=obj;
  dev->devid=devid;
  
  return 0;
}

/* Remove from global device list.
 */

static int fmn_machid_dev_remove(int p) {
  if ((p<0)||(p>=fmn_machid.devc)) return -1;
  fmn_machid_dev_cleanup(fmn_machid.devv+p);
  fmn_machid.devc--;
  memmove(fmn_machid.devv+p,fmn_machid.devv+p+1,sizeof(struct fmn_machid_dev)*(fmn_machid.devc-p));
  return 0;
}

/* Search buttons in device.
 */

static int fmn_machid_dev_search_button(struct fmn_machid_dev *dev,int btnid) {
  if (!dev) return -1;
  int lo=0,hi=dev->btnc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (btnid<dev->btnv[ck].btnid) hi=ck;
    else if (btnid>dev->btnv[ck].btnid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Add button record to device.
 */

static int fmn_machid_dev_insert_button(struct fmn_machid_dev *dev,int p,int btnid,int usage,int lo,int hi,int v) {
  if (!dev||(p<0)||(p>dev->btnc)) return -1;
  
  if (dev->btnc>=dev->btna) {
    int na=dev->btna+8;
    if (na>INT_MAX/sizeof(struct fmn_machid_btn)) return -1;
    void *nv=realloc(dev->btnv,sizeof(struct fmn_machid_btn)*na);
    if (!nv) return -1;
    dev->btnv=nv;
    dev->btna=na;
  }

  struct fmn_machid_btn *btn=dev->btnv+p;
  memmove(btn+1,btn,sizeof(struct fmn_machid_btn)*(dev->btnc-p));
  dev->btnc++;
  memset(btn,0,sizeof(struct fmn_machid_btn));
  btn->btnid=btnid;
  btn->usage=usage;
  btn->lo=lo;
  btn->hi=hi;
  if ((v>=lo)&&(v<=hi)) btn->value=v;
  else if ((lo<=0)&&(hi>=0)) btn->value=0;
  else if (lo>0) btn->value=lo;
  else btn->value=hi;

  /* Is this a hat switch? Our logic here is not perfect. I'm testing against my Zelda joystick. */
  if ((usage==0x00010039)&&(lo==0)&&(hi==7)) {
    btn->btnid_aux=1; // Just make it nonzero, and we will assign final IDs after reading all real buttons.
    btn->lo=INT_MIN; // Hats report a value outside the declared range for "none", which is annoying.
    btn->hi=INT_MAX; // Annoying but well within spec (see USB-HID HUT)
    btn->value=8; // Start out of range, ie (0,0)
  }

  return 0;
}

static int fmn_machid_dev_define_button(struct fmn_machid_dev *dev,int btnid,int usage,int lo,int hi,int v) {
  int p=fmn_machid_dev_search_button(dev,btnid);
  if (p>=0) return 0; // Already have this btnid, don't panic. Keep the first definition.
  return fmn_machid_dev_insert_button(dev,-p-1,btnid,usage,lo,hi,v);
}

/* Look for any nonzero (btnid_aux) and give it a positive unused ID.
 */

static int fmn_machid_unused_btnid(const struct fmn_machid_dev *dev) {
  int highest=0,i=dev->btnc;
  const struct fmn_machid_btn *btn=dev->btnv;
  for (;i-->0;btn++) {
    if (btn->btnid>=highest) highest=btn->btnid;
    if (btn->btnid_aux>=highest) highest=btn->btnid_aux;
  }
  if (highest==INT_MAX) return -1;
  return highest+1;
}

static int fmn_machid_dev_assign_aux_btnid(struct fmn_machid_dev *dev) {
  struct fmn_machid_btn *btn=dev->btnv;
  int i=dev->btnc; for (;i-->0;btn++) {
    if (!btn->btnid_aux) continue;
    if ((btn->btnid_aux=fmn_machid_unused_btnid(dev))<0) {
      // This is extremely improbable, but don't fail entirely if it happens.
      fprintf(stderr,"Ignoring hat %d due to no available aux ID.\n",btn->btnid);
      btn->btnid_aux=0;
    } else {
      //fprintf(stderr,"Assigned fake btnid %d for hat switch %d.\n",btn->btnid_aux,btn->btnid);
    }
  }
  return 0;
}

/* Welcome a new device, before exposing to user.
 */

static char *fmn_machid_dev_get_string(struct fmn_machid_dev *dev,CFStringRef key) {
  char buf[256]={0};

  CFStringRef string=IOHIDDeviceGetProperty(dev->obj,key);
  if (!string) return 0;
  if (!CFStringGetCString(string,buf,sizeof(buf),kCFStringEncodingUTF8)) return 0;
  int bufc=0; while ((bufc<sizeof(buf))&&buf[bufc]) bufc++;
  if (!bufc) return 0;

  /* Force non-G0 to space, then trim leading and trailing spaces. */
  int i; for (i=0;i<bufc;i++) {
    if ((buf[i]<0x20)||(buf[i]>0x7e)) buf[i]=0x20;
  }
  while (bufc&&(buf[bufc-1]==0x20)) bufc--;
  int leadc=0; while ((leadc<bufc)&&(buf[leadc]==0x20)) leadc++;
  if (leadc==bufc) return 0;
  
  char *dst=malloc(bufc-leadc+1);
  if (!dst) return 0;
  memcpy(dst,buf+leadc,bufc-leadc);
  dst[bufc-leadc]=0;
  return dst;
}

static int fmn_machid_dev_apply_IOHIDElement(struct fmn_machid_dev *dev,IOHIDElementRef element) {
      
  IOHIDElementCookie cookie=IOHIDElementGetCookie(element);
  CFIndex lo=IOHIDElementGetLogicalMin(element);
  CFIndex hi=IOHIDElementGetLogicalMax(element);
  if ((int)cookie<INT_MIN) cookie=INT_MIN; else if (cookie>INT_MAX) cookie=INT_MAX;
  if ((int)lo<INT_MIN) lo=INT_MIN; else if (lo>INT_MAX) lo=INT_MAX;
  if ((int)hi<INT_MIN) hi=INT_MIN; else if (hi>INT_MAX) hi=INT_MAX;
  if (lo>hi) { lo=INT_MIN; hi=INT_MAX; }
  uint16_t usagepage=IOHIDElementGetUsagePage(element);
  uint16_t usage=IOHIDElementGetUsage(element);

  IOHIDValueRef value=0;
  int v=0;
  if (IOHIDDeviceGetValue(dev->obj,element,&value)==kIOReturnSuccess) {
    v=IOHIDValueGetIntegerValue(value);
    if (v<lo) v=lo; else if (v>hi) v=hi;
  }

  //fprintf(stderr,"  cookie=%d, range=%d..%d, value=%d, usage=%04x%04x\n",(int)cookie,(int)lo,(int)hi,v,usagepage,usage);

  fmn_machid_dev_define_button(dev,cookie,(usagepage<<16)|usage,lo,hi,v);

  return 0;
}

static int fmn_machid_dev_handshake(struct fmn_machid_dev *dev,int vid,int pid,int usagepage,int usage) {

  dev->vid=vid;
  dev->pid=pid;
  dev->usagepage=usagepage;
  dev->usage=usage;

  /* Store manufacturer name, product name, and serial number if we can get them. */
  dev->mfrname=fmn_machid_dev_get_string(dev,CFSTR(kIOHIDManufacturerKey));
  dev->prodname=fmn_machid_dev_get_string(dev,CFSTR(kIOHIDProductKey));
  dev->serial=fmn_machid_dev_get_string(dev,CFSTR(kIOHIDSerialNumberKey));

  //fprintf(stderr,"mfr='%s' prod='%s' serial='%s'...\n",dev->mfrname,dev->prodname,dev->serial);

  /* Get limits and current value for each reported element. */
  CFArrayRef elements=IOHIDDeviceCopyMatchingElements(dev->obj,0,0);
  if (elements) {
    CFTypeID elemtypeid=IOHIDElementGetTypeID();
    CFIndex elemc=CFArrayGetCount(elements);
    CFIndex elemi; for (elemi=0;elemi<elemc;elemi++) {
      IOHIDElementRef element=(IOHIDElementRef)CFArrayGetValueAtIndex(elements,elemi);
      if (element&&(CFGetTypeID(element)==elemtypeid)) {
        fmn_machid_dev_apply_IOHIDElement(dev,element);
      }
    }
    CFRelease(elements);
  }

  if (fmn_machid_dev_assign_aux_btnid(dev)<0) return -1;

  return 0;
}

/* Connect device, callback from IOHIDManager.
 */

static void fmn_machid_cb_DeviceMatching(void *context,IOReturn result,void *sender,IOHIDDeviceRef obj) {
  
  int vid=dev_get_int(obj,CFSTR(kIOHIDVendorIDKey));
  int pid=dev_get_int(obj,CFSTR(kIOHIDProductIDKey));
  int usagepage=dev_get_int(obj,CFSTR(kIOHIDPrimaryUsagePageKey));
  int usage=dev_get_int(obj,CFSTR(kIOHIDPrimaryUsageKey));

  if (!fmn_machid.delegate.test_device(obj,vid,pid,usagepage,usage)) {
    IOHIDDeviceClose(obj,0);
    return;
  }
        
  int devid=fmn_machid_devid_unused();
  int p=fmn_machid_dev_search_obj(obj);
  if (p>=0) return; // PANIC! Device is already listed.
  p=-p-1;
  if (fmn_machid_dev_insert(p,obj,devid)<0) return;
  if (fmn_machid_dev_handshake(fmn_machid.devv+p,vid,pid,usagepage,usage)<0) { fmn_machid_dev_remove(p); return; }
  int err=fmn_machid.delegate.connect(devid);
  if (err<0) fmn_machid.error=err;
}

/* Disconnect device, callback from IOHIDManager.
 */

static void fmn_machid_cb_DeviceRemoval(void *context,IOReturn result,void *sender,IOHIDDeviceRef obj) {
  int p=fmn_machid_dev_search_obj(obj);
  if (p>=0) {
    int err=fmn_machid.delegate.disconnect(fmn_machid.devv[p].devid);
    if (err<0) fmn_machid.error=err;
    fmn_machid_dev_remove(p);
  }
}

/* Event callback from IOHIDManager.
 */

static void fmn_machid_axis_values_from_hat(int *x,int *y,int v) {
  *x=*y=0;
  switch (v) {
    case 0: *y=-1; break;
    case 1: *x=1; *y=-1; break;
    case 2: *x=1; break;
    case 3: *x=1; *y=1; break;
    case 4: *y=1; break;
    case 5: *x=-1; *y=1; break;
    case 6: *x=-1; break;
    case 7: *x=-1; *y=-1; break;
  }
  //fprintf(stderr,"hat(%d) => (%+d,%+d)\n",v,*x,*y);
}

static void fmn_machid_cb_InputValue(void *context,IOReturn result,void *sender,IOHIDValueRef value) {

  /* Locate device and buttons. */
  IOHIDElementRef element=IOHIDValueGetElement(value);
  int btnid=IOHIDElementGetCookie(element);
  if (btnid<0) return;
  IOHIDDeviceRef obj=IOHIDElementGetDevice(element);
  if (!obj) return;
  int p=fmn_machid_dev_search_obj(obj);
  if (p<0) return;
  struct fmn_machid_dev *dev=fmn_machid.devv+p;
  int btnp=fmn_machid_dev_search_button(dev,btnid);
  if (btnp<0) return;
  struct fmn_machid_btn *btn=dev->btnv+btnp;

  /* Clamp value and confirm it actually changed. */
  CFIndex v=IOHIDValueGetIntegerValue(value);
  //fprintf(stderr,"%d[%d..%d]\n",(int)v,btn->lo,btn->hi);
  if (v<btn->lo) v=btn->lo;
  else if (v>btn->hi) v=btn->hi;
  if (v==btn->value) return;
  int ov=btn->value;
  btn->value=v;

  /* If this is a hat switch, split into two axes. */
  if (btn->btnid_aux) {
    int ovx,ovy,nvx,nvy;
    fmn_machid_axis_values_from_hat(&ovx,&ovy,ov);
    fmn_machid_axis_values_from_hat(&nvx,&nvy,v);
    if (ovx!=nvx) {
      int err=fmn_machid.delegate.button(dev->devid,btnid,nvx);
      if (err<0) fmn_machid.error=err;
    }
    if (ovy!=nvy) {
      int err=fmn_machid.delegate.button(dev->devid,btn->btnid_aux,nvy);
      if (err<0) fmn_machid.error=err;
    }
    return;
  }

  /* Report ordinary scalar buttons. */
  int err=fmn_machid.delegate.button(dev->devid,btnid,v);
  if (err<0) fmn_machid.error=err;
}

/* Default delegate callbacks.
 */
 
static int fmn_machid_delegate_test_device_default(void *hiddevice,int vid,int pid,int usagepage,int usage) {
  if (usagepage==0x0001) switch (usage) { // Usage Page 0x0001: Generic Desktop Controls
    case 0x0004: return 1; // ...0x0004: Joystick
    case 0x0005: return 1; // ...0x0005: Game Pad
    case 0x0008: return 1; // ...0x0008: Multi-axis Controller
  }
  return 0;
}

static int fmn_machid_delegate_connect_default(int devid) {
  return 0;
}

static int fmn_machid_delegate_disconnect_default(int devid) {
  return 0;
}

static int fmn_machid_delegate_button_default(int devid,int btnid,int value) {
  return 0;
}

/* Init.
 */

int fmn_machid_init(const struct fmn_machid_delegate *delegate) {
  if (fmn_machid.hidmgr) return -1; // Already initialized.
  memset(&fmn_machid,0,sizeof(fmn_machid));
  
  if (!(fmn_machid.hidmgr=IOHIDManagerCreate(kCFAllocatorDefault,0))) return -1;

  if (delegate) memcpy(&fmn_machid.delegate,delegate,sizeof(struct fmn_machid_delegate));
  if (!fmn_machid.delegate.test_device) fmn_machid.delegate.test_device=fmn_machid_delegate_test_device_default;
  if (!fmn_machid.delegate.connect) fmn_machid.delegate.connect=fmn_machid_delegate_connect_default;
  if (!fmn_machid.delegate.disconnect) fmn_machid.delegate.disconnect=fmn_machid_delegate_disconnect_default;
  if (!fmn_machid.delegate.button) fmn_machid.delegate.button=fmn_machid_delegate_button_default;

  IOHIDManagerRegisterDeviceMatchingCallback(fmn_machid.hidmgr,fmn_machid_cb_DeviceMatching,0);
  IOHIDManagerRegisterDeviceRemovalCallback(fmn_machid.hidmgr,fmn_machid_cb_DeviceRemoval,0);
  IOHIDManagerRegisterInputValueCallback(fmn_machid.hidmgr,fmn_machid_cb_InputValue,0);

  IOHIDManagerSetDeviceMatching(fmn_machid.hidmgr,0); // match every HID

  IOHIDManagerScheduleWithRunLoop(fmn_machid.hidmgr,CFRunLoopGetCurrent(),AKMACHID_RUNLOOP_MODE);
    
  if (IOHIDManagerOpen(fmn_machid.hidmgr,0)<0) {
    IOHIDManagerUnscheduleFromRunLoop(fmn_machid.hidmgr,CFRunLoopGetCurrent(),AKMACHID_RUNLOOP_MODE);
    IOHIDManagerClose(fmn_machid.hidmgr,0);
    memset(&fmn_machid,0,sizeof(fmn_machid));
    return -1;
  }
  
  return 0;
}

/* Quit.
 */

void fmn_machid_quit() {
  if (!fmn_machid.hidmgr) return;
  IOHIDManagerUnscheduleFromRunLoop(fmn_machid.hidmgr,CFRunLoopGetCurrent(),AKMACHID_RUNLOOP_MODE);
  IOHIDManagerClose(fmn_machid.hidmgr,0);
  if (fmn_machid.devv) {
    while (fmn_machid.devc--) fmn_machid_dev_cleanup(fmn_machid.devv+fmn_machid.devc);
    free(fmn_machid.devv);
  }
  memset(&fmn_machid,0,sizeof(fmn_machid));
}

/* Update.
 */

int fmn_machid_update(double timeout) {
  if (!fmn_machid.hidmgr) return -1;
  CFRunLoopRunInMode(AKMACHID_RUNLOOP_MODE,timeout,0);
  if (fmn_machid.error) {
    int result=fmn_machid.error;
    fmn_machid.error=0;
    return result;
  }
  return 0;
}

/* Trivial public device accessors.
 */

int fmn_machid_count_devices() {
  return fmn_machid.devc;
}

int fmn_machid_devid_for_index(int index) {
  if ((index<0)||(index>=fmn_machid.devc)) return 0;
  return fmn_machid.devv[index].devid;
}

int fmn_machid_index_for_devid(int devid) {
  return fmn_machid_dev_search_devid(devid);
}

#define GETINT(fldname) { \
  int p=fmn_machid_dev_search_devid(devid); \
  if (p<0) return 0; \
  return fmn_machid.devv[p].fldname; \
}
#define GETSTR(fldname) { \
  int p=fmn_machid_dev_search_devid(devid); \
  if (p<0) return ""; \
  if (!fmn_machid.devv[p].fldname) return ""; \
  return fmn_machid.devv[p].fldname; \
}
 
void *fmn_machid_dev_get_IOHIDDeviceRef(int devid) GETINT(obj)
int fmn_machid_dev_get_vendor_id(int devid) GETINT(vid)
int fmn_machid_dev_get_product_id(int devid) GETINT(pid)
int fmn_machid_dev_get_usage_page(int devid) GETINT(usagepage)
int fmn_machid_dev_get_usage(int devid) GETINT(usage)
const char *fmn_machid_dev_get_manufacturer_name(int devid) GETSTR(mfrname)
const char *fmn_machid_dev_get_product_name(int devid) GETSTR(prodname)
const char *fmn_machid_dev_get_serial_number(int devid) GETSTR(serial)

#undef GETINT
#undef GETSTR

/* Count buttons.
 * We need to report the 'aux' as distinct buttons.
 */

int fmn_machid_dev_count_buttons(int devid) {
  int devp=fmn_machid_dev_search_devid(devid);
  if (devp<0) return 0;
  const struct fmn_machid_dev *dev=fmn_machid.devv+devp;
  int btnc=dev->btnc;
  const struct fmn_machid_btn *btn=dev->btnv;
  int i=dev->btnc; for (;i-->0;btn++) {
    if (btn->btnid_aux) btnc++;
  }
  return btnc;
}

/* Get button properties.
 */

int fmn_machid_dev_get_button_info(int *btnid,int *usage,int *lo,int *hi,int *value,int devid,int index) {
  if (index<0) return -1;
  int devp=fmn_machid_dev_search_devid(devid);
  if (devp<0) return -1;
  struct fmn_machid_dev *dev=fmn_machid.devv+devp;

  /* Scalar or hat X, the principal button. */
  if (index<dev->btnc) {
    struct fmn_machid_btn *btn=dev->btnv+index;
    if (btnid) *btnid=btn->btnid;
    if (usage) *usage=btn->usage;
    if (btn->btnid_aux) {
      if (lo) *lo=-1;
      if (hi) *hi=1;
      if (value) switch (btn->value) {
        case 1: case 2: case 3: *value=1; break;
        case 5: case 6: case 7: *value=-1; break;
        default: *value=0;
      }
    } else {
      if (lo) *lo=btn->lo;
      if (hi) *hi=btn->hi;
      if (value) *value=btn->value;
    }
    return 0;
  }

  /* Is this an 'aux' button? */
  index-=dev->btnc;
  const struct fmn_machid_btn *btn=dev->btnv;
  int i=dev->btnc; for (;i-->0;btn++) {
    if (!btn->btnid_aux) continue;
    if (!index--) {
      if (btnid) *btnid=btn->btnid_aux;
      if (usage) *usage=btn->usage;
      if (lo) *lo=-1;
      if (hi) *hi=1;
      if (value) switch (btn->value) {
        case 7: case 0: case 1: *value=-1; break;
        case 3: case 4: case 5: *value=1; break;
        default: *value=0;
      }
      return 0;
    }
  }

  return -1;
}
