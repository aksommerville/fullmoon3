#ifndef FMN_GLX_INTERNAL_H
#define FMN_GLX_INTERNAL_H

#include "opt/hw/fmn_hw.h"
#include "game/image/fmn_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <GL/gl.h>

// Required only for making intelligent initial-size decisions in a multi-monitor setting.
// apt install libxinerama-dev
#ifndef FMN_USE_xinerama
  #define FMN_USE_xinerama 1
#endif
#if FMN_USE_xinerama
  #include <X11/extensions/Xinerama.h>
#endif

#define KeyRepeat (LASTEvent+2)
#define FMN_GLX_KEY_REPEAT_INTERVAL 10
#define FMN_GLX_ICON_SIZE_LIMIT 64

struct fmn_hw_video_glx {
  struct fmn_hw_video hdr;
  
  Display *dpy;
  int screen;
  Window win;
  GLXContext ctx;
  int glx_version_minor,glx_version_major;
  
  Atom atom_WM_PROTOCOLS;
  Atom atom_WM_DELETE_WINDOW;
  Atom atom__NET_WM_STATE;
  Atom atom__NET_WM_STATE_FULLSCREEN;
  Atom atom__NET_WM_STATE_ADD;
  Atom atom__NET_WM_STATE_REMOVE;
  Atom atom__NET_WM_ICON;
  Atom atom__NET_WM_ICON_NAME;
  Atom atom__NET_WM_NAME;
  Atom atom_WM_CLASS;
  Atom atom_STRING;
  Atom atom_UTF8_STRING;
  
  int screensaver_suppressed;
  int focus;
  
  GLuint fbtexid;
  struct fmn_image *fb;
  uint8_t *fbscratch; // for fb formats we need to convert before upload
  int dstdirty;
  int dstx,dsty,dstw,dsth;
};

#define VIDEO ((struct fmn_hw_video_glx*)video)

int fmn_glx_update(struct fmn_hw_video *video);

int fmn_glx_codepoint_from_keysym(int keysym);
int fmn_glx_usb_usage_from_keysym(int keysym);

#endif
