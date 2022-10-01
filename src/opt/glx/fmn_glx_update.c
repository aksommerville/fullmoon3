#include "fmn_glx_internal.h"

/* Key press, release, or repeat.
 */
 
static int fmn_glx_evt_key(struct fmn_hw_video *video,XKeyEvent *evt,int value) {

  /* Pass the raw keystroke. */
  if (video->delegate->key) {
    KeySym keysym=XkbKeycodeToKeysym(VIDEO->dpy,evt->keycode,0,0);
    if (keysym) {
      int keycode=fmn_glx_usb_usage_from_keysym((int)keysym);
      if (keycode) {
        int err=video->delegate->key(video,keycode,value);
        if (err) return err; // Stop here if acknowledged.
      }
    }
  }
  
  /* Pass text if press or repeat, and text can be acquired. */
  if (video->delegate->text) {
    int shift=(evt->state&ShiftMask)?1:0;
    KeySym tkeysym=XkbKeycodeToKeysym(VIDEO->dpy,evt->keycode,0,shift);
    if (shift&&!tkeysym) { // If pressing shift makes this key "not a key anymore", fuck that and pretend shift is off
      tkeysym=XkbKeycodeToKeysym(VIDEO->dpy,evt->keycode,0,0);
    }
    if (tkeysym) {
      int codepoint=fmn_glx_codepoint_from_keysym(tkeysym);
      if (codepoint && (evt->type == KeyPress || evt->type == KeyRepeat)) {
        video->delegate->text(video,codepoint);
      }
    }
  }
  
  return 0;
}

/* Mouse events.
 */
 
static int fmn_glx_evt_mbtn(struct fmn_hw_video *video,XButtonEvent *evt,int value) {
  switch (evt->button) {
    case 1: if (video->delegate->mbutton) video->delegate->mbutton(video,1,value); break;
    case 2: if (video->delegate->mbutton) video->delegate->mbutton(video,3,value); break;
    case 3: if (video->delegate->mbutton) video->delegate->mbutton(video,2,value); break;
    case 4: if (value&&video->delegate->mwheel) video->delegate->mwheel(video,0,-1); break;
    case 5: if (value&&video->delegate->mwheel) video->delegate->mwheel(video,0,1); break;
    case 6: if (value&&video->delegate->mwheel) video->delegate->mwheel(video,-1,0); break;
    case 7: if (value&&video->delegate->mwheel) video->delegate->mwheel(video,1,0); break;
  }
  return 0;
}

static int fmn_glx_evt_mmotion(struct fmn_hw_video *video,XMotionEvent *evt) {
  if (video->delegate->mmotion) {
    video->delegate->mmotion(video,evt->x,evt->y);
  }
  return 0;
}

/* Client message.
 */
 
static int fmn_glx_evt_client(struct fmn_hw_video *video,XClientMessageEvent *evt) {
  if (evt->message_type==VIDEO->atom_WM_PROTOCOLS) {
    if (evt->format==32) {
      if (evt->data.l[0]==VIDEO->atom_WM_DELETE_WINDOW) {
        if (video->delegate->close) {
          video->delegate->close(video);
        }
      }
    }
  }
  return 0;
}

/* Configuration event (eg resize).
 */
 
static int fmn_glx_evt_configure(struct fmn_hw_video *video,XConfigureEvent *evt) {
  int nw=evt->width,nh=evt->height;
  if ((nw!=video->winw)||(nh!=video->winh)) {
    VIDEO->dstdirty=1;
    video->winw=nw;
    video->winh=nh;
    if (video->delegate->resize) {
      video->delegate->resize(video,nw,nh);
    }
  }
  return 0;
}

/* Focus.
 */
 
static int fmn_glx_evt_focus(struct fmn_hw_video *video,XFocusInEvent *evt,int value) {
  if (value==VIDEO->focus) return 0;
  VIDEO->focus=value;
  if (video->delegate->focus) {
    video->delegate->focus(video,value);
  }
  return 0;
}

/* Process one event.
 */
 
static int fmn_glx_receive_event(struct fmn_hw_video *video,XEvent *evt) {
  if (!evt) return -1;
  switch (evt->type) {
  
    case KeyPress: return fmn_glx_evt_key(video,&evt->xkey,1);
    case KeyRelease: return fmn_glx_evt_key(video,&evt->xkey,0);
    case KeyRepeat: return fmn_glx_evt_key(video,&evt->xkey,2);
    
    case ButtonPress: return fmn_glx_evt_mbtn(video,&evt->xbutton,1);
    case ButtonRelease: return fmn_glx_evt_mbtn(video,&evt->xbutton,0);
    case MotionNotify: return fmn_glx_evt_mmotion(video,&evt->xmotion);
    
    case ClientMessage: return fmn_glx_evt_client(video,&evt->xclient);
    
    case ConfigureNotify: return fmn_glx_evt_configure(video,&evt->xconfigure);
    
    case FocusIn: return fmn_glx_evt_focus(video,&evt->xfocus,1);
    case FocusOut: return fmn_glx_evt_focus(video,&evt->xfocus,0);
    
  }
  return 0;
}

/* Update.
 */
 
int fmn_glx_update(struct fmn_hw_video *video) {
  int evtc=XEventsQueued(VIDEO->dpy,QueuedAfterFlush);
  while (evtc-->0) {
    XEvent evt={0};
    XNextEvent(VIDEO->dpy,&evt);
    if ((evtc>0)&&(evt.type==KeyRelease)) {
      XEvent next={0};
      XNextEvent(VIDEO->dpy,&next);
      evtc--;
      if ((next.type==KeyPress)&&(evt.xkey.keycode==next.xkey.keycode)&&(evt.xkey.time>=next.xkey.time-FMN_GLX_KEY_REPEAT_INTERVAL)) {
        evt.type=KeyRepeat;
        if (fmn_glx_receive_event(video,&evt)<0) return -1;
      } else {
        if (fmn_glx_receive_event(video,&evt)<0) return -1;
        if (fmn_glx_receive_event(video,&next)<0) return -1;
      }
    } else {
      if (fmn_glx_receive_event(video,&evt)<0) return -1;
    }
  }
  return 0;
}
