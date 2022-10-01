#include "fmn_inmgr_internal.h"

/* Cleanup.
 */
 
void fmn_inmgr_device_cleanup(struct fmn_inmgr_device *device) {
  if (device->buttonv) free(device->buttonv);
}

void fmn_inmgr_del(struct fmn_inmgr *inmgr) {
  if (!inmgr) return;
  if (inmgr->devicev) {
    while (inmgr->devicec-->0) fmn_inmgr_device_cleanup(inmgr->devicev+inmgr->devicec);
    free(inmgr->devicev);
  }
  free(inmgr);
}

/* New.
 */

struct fmn_inmgr *fmn_inmgr_new(const struct fmn_inmgr_delegate *delegate) {
  struct fmn_inmgr *inmgr=calloc(1,sizeof(struct fmn_inmgr));
  if (!inmgr) return 0;
  if (delegate) inmgr->delegate=*delegate;
  return inmgr;
}

/* Trivial accessors.
 */
 
void *fmn_inmgr_get_userdata(const struct fmn_inmgr *inmgr) {
  if (!inmgr) return 0;
  return inmgr->delegate.userdata;
}

/* Device list.
 */

int fmn_inmgr_devicev_search(const struct fmn_inmgr *inmgr,int devid) {
  int lo=0,hi=inmgr->devicec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (devid<inmgr->devicev[ck].devid) hi=ck;
    else if (devid>inmgr->devicev[ck].devid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

struct fmn_inmgr_device *fmn_inmgr_devicev_insert(struct fmn_inmgr *inmgr,int p,int devid) {
  if ((p<0)||(p>inmgr->devicec)) return 0;
  if (p&&(devid<=inmgr->devicev[p-1].devid)) return 0;
  if ((p<inmgr->devicec)&&(devid>=inmgr->devicev[p].devid)) return 0;
  
  if (inmgr->devicec>=inmgr->devicea) {
    int na=inmgr->devicea+8;
    if (na>INT_MAX/sizeof(struct fmn_inmgr_device)) return 0;
    void *nv=realloc(inmgr->devicev,sizeof(struct fmn_inmgr_device)*na);
    if (!nv) return 0;
    inmgr->devicev=nv;
    inmgr->devicea=na;
  }
  
  struct fmn_inmgr_device *device=inmgr->devicev+p;
  memmove(device+1,device,sizeof(struct fmn_inmgr_device)*(inmgr->devicec-p));
  inmgr->devicec++;
  memset(device,0,sizeof(struct fmn_inmgr_device));
  device->devid=devid;
  
  return device;
}

/* Button list in device.
 */

int fmn_inmgr_device_buttonv_search(const struct fmn_inmgr_device *device,int srcbtnid) {
  int lo=0,hi=device->buttonc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (srcbtnid<device->buttonv[ck].srcbtnid) hi=ck;
    else if (srcbtnid>device->buttonv[ck].srcbtnid) lo=ck+1;
    else {
      while ((ck>lo)&&(device->buttonv[ck-1].srcbtnid==srcbtnid)) ck--;
      return ck;
    }
  }
  return -lo-1;
}

struct fmn_inmgr_device_button *fmn_inmgr_device_buttonv_insert(struct fmn_inmgr_device *device,int p,int srcbtnid) {
  if ((p<0)||(p>device->buttonc)) return 0;
  if (p&&(srcbtnid<device->buttonv[p-1].srcbtnid)) return 0;
  if ((p<device->buttonc)&&(srcbtnid>device->buttonv[p].srcbtnid)) return 0;
  
  if (device->buttonc>=device->buttona) {
    int na=device->buttona+16;
    if (na>INT_MAX/sizeof(struct fmn_inmgr_device_button)) return 0;
    void *nv=realloc(device->buttonv,sizeof(struct fmn_inmgr_device_button)*na);
    if (!nv) return 0;
    device->buttonv=nv;
    device->buttona=na;
  }
  
  struct fmn_inmgr_device_button *button=device->buttonv+p;
  memmove(button+1,button,sizeof(struct fmn_inmgr_device_button)*(device->buttonc-p));
  device->buttonc++;
  memset(button,0,sizeof(struct fmn_inmgr_device_button));
  button->srcbtnid=srcbtnid;
  
  return button;
}
