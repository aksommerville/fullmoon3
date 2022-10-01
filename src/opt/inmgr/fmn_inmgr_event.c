#include "fmn_inmgr_internal.h"

/* Clear state, top level only, and notify delegate.
 */
 
void fmn_inmgr_clear_state(struct fmn_inmgr *inmgr) {
  if (!inmgr->state) return;
  uint8_t pv=inmgr->state;
  inmgr->state=0;
  if (!inmgr->delegate.state) return;
  uint8_t mask=0x80;
  for (;mask;mask>>=1) if (pv&mask) {
    inmgr->delegate.state(inmgr,mask,0,0);
  }
}

/* Add buttons to device.
 */

static int fmn_inmgr_device_add_single(struct fmn_inmgr_device *device,int srcbtnid,int dstbtnid,int fulllo,int fullhi) {
  int p=fmn_inmgr_device_buttonv_search(device,srcbtnid);
  if (p<0) p=-p-1;
  struct fmn_inmgr_device_button *button=fmn_inmgr_device_buttonv_insert(device,p,srcbtnid);
  if (!button) return -1;
  button->dstbtnid=dstbtnid;
  if (fulllo<0) {
    button->srclo=0;
    button->srchi=INT_MAX;
  } else {
    button->srclo=(fulllo+fullhi)>>1;
    if (button->srclo<=fulllo) button->srclo++;
    button->srchi=INT_MAX;
  }
  return 0;
}

// (dstbtnid) must be (FMN_BUTTON_LEFT|FMN_BUTTON_RIGHT) or (FMN_BUTTON_UP|FMN_BUTTON_DOWN).
static int fmn_inmgr_device_add_double(struct fmn_inmgr_device *device,int srcbtnid,int dstbtnid,int fulllo,int fullhi) {
  if (fulllo>fullhi-2) return 0;
  int mid=(fulllo+fullhi)>>1;
  int p=fmn_inmgr_device_buttonv_search(device,srcbtnid);
  if (p<0) p=-p-1;
  struct fmn_inmgr_device_button *button=fmn_inmgr_device_buttonv_insert(device,p++,srcbtnid);
  if (!button) return -1;
  button->dstbtnid=dstbtnid&(FMN_BUTTON_LEFT|FMN_BUTTON_UP);
  button->srclo=INT_MIN;
  button->srchi=(fulllo+mid)>>1;
  if (button->srchi>=mid) button->srchi--;
  if (!(button=fmn_inmgr_device_buttonv_insert(device,p,srcbtnid))) return -1;
  button->dstbtnid=dstbtnid&(FMN_BUTTON_RIGHT|FMN_BUTTON_DOWN);
  button->srchi=INT_MAX;
  button->srclo=(fullhi+mid)>>1;
  if (button->srclo<=mid) button->srclo++;
  return 0;
}

//TODO hats as dpad

/* Configure for one button in device.
 */
 
struct fmn_inmgr_device_configure_context {
  struct fmn_inmgr *inmgr;
  struct fmn_inmgr_device *device;
};
 
static int fmn_inmgr_device_configure_hw_button(
  struct fmn_hw_input *input,int devid,int btnid,int usage,int lo,int hi,int value,void *userdata
) {
  struct fmn_inmgr_device_configure_context *ctx=userdata;
  //if ((usage<0x00070000)||(usage>=0x00080000)) fprintf(stderr,"%s %d 0x%08x %d..%d =%d\n",__func__,btnid,usage,lo,hi,value);
  
  //TODO If there's a template, use it.
  
  // Generic and hard-coded maps.
  int dstbtnid=0;
  switch (usage) {
    #define SINGLE(u,dstbtnid) case u: return fmn_inmgr_device_add_single(ctx->device,btnid,dstbtnid,lo,hi);
    #define HORZ(u) case u: return fmn_inmgr_device_add_double(ctx->device,btnid,FMN_BUTTON_LEFT|FMN_BUTTON_RIGHT,lo,hi);
    #define VERT(u) case u: return fmn_inmgr_device_add_double(ctx->device,btnid,FMN_BUTTON_UP|FMN_BUTTON_DOWN,lo,hi);
    
    // Keyboard.
    SINGLE(0x00070004,FMN_BUTTON_LEFT) // a
    SINGLE(0x00070007,FMN_BUTTON_RIGHT) // d
    SINGLE(0x00070016,FMN_BUTTON_DOWN) // s
    SINGLE(0x0007001a,FMN_BUTTON_UP) // w
    SINGLE(0x0007001b,FMN_BUTTON_B) // x
    SINGLE(0x0007001d,FMN_BUTTON_A) // z
    SINGLE(0x00070028,FMN_BUTTON_B) // enter
    SINGLE(0x00070029,FMN_INMGR_ACTION_QUIT) // esc
    SINGLE(0x0007002c,FMN_BUTTON_A) // space
    SINGLE(0x00070036,FMN_BUTTON_B) // comma
    SINGLE(0x00070037,FMN_BUTTON_A) // dot
    SINGLE(0x00070044,FMN_INMGR_ACTION_FULLSCREEN) // f11
    SINGLE(0x0007004f,FMN_BUTTON_RIGHT) // right
    SINGLE(0x00070050,FMN_BUTTON_LEFT) // left
    SINGLE(0x00070051,FMN_BUTTON_DOWN) // down
    SINGLE(0x00070052,FMN_BUTTON_UP) // up
    SINGLE(0x00070058,FMN_BUTTON_B) // kp enter
    SINGLE(0x0007005a,FMN_BUTTON_DOWN) // kp 2
    SINGLE(0x0007005c,FMN_BUTTON_LEFT) // kp 4
    SINGLE(0x0007005d,FMN_BUTTON_DOWN) // kp 5
    SINGLE(0x0007005e,FMN_BUTTON_RIGHT) // kp 6
    SINGLE(0x00070060,FMN_BUTTON_UP) // kp 8
    SINGLE(0x00070062,FMN_BUTTON_A) // kp 0
    
    HORZ(0x00010030)
    VERT(0x00010031)
    HORZ(0x00010033)
    VERT(0x00010035)
    
    #undef SINGLE
    #undef HORZ
    #undef VERT
  }
  if ((usage>=0x00090000)&&(usage<0x000a0000)) {
    if (usage&1) return fmn_inmgr_device_add_single(ctx->device,btnid,FMN_BUTTON_A,lo,hi);
    return fmn_inmgr_device_add_single(ctx->device,btnid,FMN_BUTTON_B,lo,hi);
  }
  return 0;
}

/* Configure device from hw_input.
 */
 
static int fmn_inmgr_device_configure_hw(struct fmn_inmgr *inmgr,struct fmn_inmgr_device *device,struct fmn_hw_input *input) {

  int vid=0,pid=0;
  const char *name=fmn_hw_input_get_ids(&vid,&pid,input,device->devid);
  if (!name) name="(unknown)";
  fprintf(stderr,"Configuring input device %04x:%04x '%s'\n",vid,pid,name);
  
  //TODO Look up (vid,pid,name) in the mapping templates that we don't have yet.
  
  struct fmn_inmgr_device_configure_context ctx={
    .inmgr=inmgr,
    .device=device,
  };
  if (fmn_hw_input_enumerate(input,device->devid,fmn_inmgr_device_configure_hw_button,&ctx)<0) return -1;
  
  return 0;
}

/* Configure device for system keyboard.
 */
 
static int fmn_inmgr_device_configure_keyboard(struct fmn_inmgr *inmgr,struct fmn_inmgr_device *device) {

  //TODO Look up mapping template.
  
  struct fmn_inmgr_device_configure_context ctx={
    .inmgr=inmgr,
    .device=device,
  };
  int btnid;
  for (btnid=0x00070004;btnid<=0x00070063;btnid++) if (fmn_inmgr_device_configure_hw_button(0,device->devid,btnid,btnid,0,1,0,&ctx)<0) return -1;
  for (btnid=0x000700e0;btnid<=0x000700e7;btnid++) if (fmn_inmgr_device_configure_hw_button(0,device->devid,btnid,btnid,0,1,0,&ctx)<0) return -1;
  // It's a hard-coded mapping, so we could just add it verbatim.
  // But this ^ is ths strategy we're going to need, once we enable configurable mapping.
  
  return 0;
}

/* Connect device.
 */
 
int fmn_inmgr_connect(struct fmn_inmgr *inmgr,struct fmn_hw_input *input,int devid) {

  int p=fmn_inmgr_devicev_search(inmgr,devid);
  if (p>=0) {
    // We already have it mapped? This should never happen.
    // But if it does, drop the old one and carry on.
    struct fmn_inmgr_device *device=inmgr->devicev+p;
    fmn_inmgr_device_cleanup(device);
    inmgr->devicec--;
    memmove(device,device+1,sizeof(struct fmn_inmgr_device)*(inmgr->devicec-p));
    fmn_inmgr_clear_state(inmgr);
  } else {
    p=-p-1;
  }
  struct fmn_inmgr_device *device=fmn_inmgr_devicev_insert(inmgr,p,devid);
  if (!device) return -1;
  
  // Null (input) means system keyboard. Anything else, use hw_input facilities.
  if (input) {
    if (fmn_inmgr_device_configure_hw(inmgr,device,input)<0) return -1;
  } else {
    if (fmn_inmgr_device_configure_keyboard(inmgr,device)<0) return -1;
  }
  
  return 0;
}

/* Disconnect device.
 */
 
int fmn_inmgr_disconnect(struct fmn_inmgr *inmgr,int devid) {

  int p=fmn_inmgr_devicev_search(inmgr,devid);
  if (p<0) return 0;
  struct fmn_inmgr_device *device=inmgr->devicev+p;
  
  uint8_t devstate=device->state;
  inmgr->devicec--;
  fmn_inmgr_device_cleanup(device);
  memmove(device,device+1,sizeof(struct fmn_inmgr_device)*(inmgr->devicec-p));
  
  if (devstate&&inmgr->delegate.state) {
    uint8_t bit=0x80;
    for (;bit;bit>>=1) {
      if (!(devstate&bit)) continue;
      if (!(inmgr->state&bit)) continue;
      inmgr->state&=~bit;
      inmgr->delegate.state(inmgr,bit,0,inmgr->state);
    }
  }
  
  return 0;
}

/* Raw input event.
 */
 
int fmn_inmgr_button(struct fmn_inmgr *inmgr,int devid,int btnid,int value) {

  int devp=fmn_inmgr_devicev_search(inmgr,devid);
  if (devp<0) return 0;
  struct fmn_inmgr_device *device=inmgr->devicev+devp;
  
  int btnp=fmn_inmgr_device_buttonv_search(device,btnid);
  if (btnp<0) return 0;
  
  struct fmn_inmgr_device_button *button=device->buttonv+btnp;
  for (;(btnp<device->buttonc)&&(button->srcbtnid==btnid);btnp++,button++) {
    if (value==button->srcvalue) continue;
    button->srcvalue=value;
    int dstvalue=((value>=button->srclo)&&(value<=button->srchi))?1:0;
    if (dstvalue==button->dstvalue) continue;
    button->dstvalue=dstvalue;
    
    if (button->dstbtnid>=0x100) {
      if (dstvalue) {
        if (inmgr->delegate.action) inmgr->delegate.action(inmgr,button->dstbtnid);
      }
    } else if (button->dstvalue) {
      if (!(device->state&button->dstbtnid)) {
        device->state|=button->dstbtnid;
        if (!(inmgr->state&button->dstbtnid)) {
          inmgr->state|=button->dstbtnid;
          if (inmgr->delegate.state) inmgr->delegate.state(inmgr,button->dstbtnid,1,inmgr->state);
        }
      }
    } else {
      if (device->state&button->dstbtnid) {
        device->state&=~button->dstbtnid;
        if (inmgr->state&button->dstbtnid) {
          inmgr->state&=~button->dstbtnid;
          if (inmgr->delegate.state) inmgr->delegate.state(inmgr,button->dstbtnid,0,inmgr->state);
        }
      }
    }
  }
  
  return 0;
}

/* Premapped input event.
 */
 
int fmn_inmgr_premapped(struct fmn_inmgr *inmgr,int devid,uint8_t btnid,int value) {
  int devp=fmn_inmgr_devicev_search(inmgr,devid);
  if (devp>=0) {
    struct fmn_inmgr_device *device=inmgr->devicev+devp;
    if (value) {
      if (device->state&btnid) return 0;
      device->state|=btnid;
    } else {
      if (!(device->state&btnid)) return 0;
      device->state&=~btnid;
    }
  }
  if (value) {
    if (inmgr->state&btnid) return 0;
    inmgr->state|=btnid;
    if (inmgr->delegate.state) inmgr->delegate.state(inmgr,btnid,1,inmgr->state);
  } else {
    if (!(inmgr->state&btnid)) return 0;
    inmgr->state&=~btnid;
    if (inmgr->delegate.state) inmgr->delegate.state(inmgr,btnid,0,inmgr->state);
  }
  return 0;
}
