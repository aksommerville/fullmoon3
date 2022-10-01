#include "fmn_glx_internal.h"

/* Cleanup.
 */
 
static void _glx_del(struct fmn_hw_video *video) {
  fmn_image_del(VIDEO->fb);
  if (VIDEO->fbscratch) free(VIDEO->fbscratch);
  if (VIDEO->fbtexid) {
    glDeleteTextures(1,&VIDEO->fbtexid);
  }
  if (VIDEO->ctx) {
    glXMakeCurrent(VIDEO->dpy,0,0);
    glXDestroyContext(VIDEO->dpy,VIDEO->ctx);
  }
  if (VIDEO->dpy) {
    XCloseDisplay(VIDEO->dpy);
  }
}

/* Set window icon.
 */
 
static void fmn_glx_copy_pixels(long *dst,const uint8_t *src,int c) {
  for (;c-->0;dst++,src+=4) {
    *dst=(src[3]<<24)|(src[0]<<16)|(src[1]<<8)|src[2];
  }
}
 
static void fmn_glx_set_icon(struct fmn_hw_video *video,const void *rgba,int w,int h) {
  if (!rgba||(w<1)||(h<1)||(w>FMN_GLX_ICON_SIZE_LIMIT)||(h>FMN_GLX_ICON_SIZE_LIMIT)) return;
  int length=2+w*h;
  long *pixels=malloc(sizeof(long)*length);
  if (!pixels) return;
  pixels[0]=w;
  pixels[1]=h;
  fmn_glx_copy_pixels(pixels+2,rgba,w*h);
  XChangeProperty(VIDEO->dpy,VIDEO->win,VIDEO->atom__NET_WM_ICON,XA_CARDINAL,32,PropModeReplace,(unsigned char*)pixels,length);
  free(pixels);
}

/* Get the size of the monitor we're going to display on.
 * NOT the size of the logical desktop, if it can be avoided.
 * We don't actually know which monitor will be chosen, and we don't want to force it, so use the smallest.
 */
 
static void fmn_glx_estimate_monitor_size(int *w,int *h,struct fmn_hw_video *video) {
  *w=*h=0;
  #if FMN_USE_xinerama
    int infoc=0;
    XineramaScreenInfo *infov=XineramaQueryScreens(VIDEO->dpy,&infoc);
    if (infov) {
      if (infoc>0) {
        *w=infov[0].width;
        *h=infov[0].height;
        int i=infoc; while (i-->1) {
          if ((infov[i].width<*w)||(infov[i].height<*h)) {
            *w=infov[i].width;
            *h=infov[i].height;
          }
        }
      }
      XFree(infov);
    }
  #endif
  if ((*w<1)||(*h<1)) {
    *w=DisplayWidth(VIDEO->dpy,0);
    *h=DisplayHeight(VIDEO->dpy,0);
    if ((*w<1)||(*h<1)) {
      *w=640;
      *h=480;
    }
  }
}

/* Initialize display and atoms.
 */
 
static int fmn_glx_init_display(struct fmn_hw_video *video) {
  
  if (!(VIDEO->dpy=XOpenDisplay(0))) return -1;
  VIDEO->screen=DefaultScreen(VIDEO->dpy);

  #define GETATOM(tag) VIDEO->atom_##tag=XInternAtom(VIDEO->dpy,#tag,0);
  GETATOM(WM_PROTOCOLS)
  GETATOM(WM_DELETE_WINDOW)
  GETATOM(_NET_WM_STATE)
  GETATOM(_NET_WM_STATE_FULLSCREEN)
  GETATOM(_NET_WM_STATE_ADD)
  GETATOM(_NET_WM_STATE_REMOVE)
  GETATOM(_NET_WM_ICON)
  GETATOM(_NET_WM_ICON_NAME)
  GETATOM(_NET_WM_NAME)
  GETATOM(STRING)
  GETATOM(UTF8_STRING)
  GETATOM(WM_CLASS)
  #undef GETATOM
  
  return 0;
}

/* Get GLX framebuffer config.
 */
 
static GLXFBConfig fmn_glx_get_fbconfig(struct fmn_hw_video *video) {

  int attrv[]={
    GLX_X_RENDERABLE,1,
    GLX_DRAWABLE_TYPE,GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE,GLX_TRUE_COLOR,
    GLX_RED_SIZE,8,
    GLX_GREEN_SIZE,8,
    GLX_BLUE_SIZE,8,
    GLX_ALPHA_SIZE,0,
    GLX_DEPTH_SIZE,0,
    GLX_STENCIL_SIZE,0,
    GLX_DOUBLEBUFFER,0,
  0};
  
  if (!glXQueryVersion(VIDEO->dpy,&VIDEO->glx_version_major,&VIDEO->glx_version_minor)) {
    return (GLXFBConfig){0};
  }
  
  int fbc=0;
  GLXFBConfig *configv=glXChooseFBConfig(VIDEO->dpy,VIDEO->screen,attrv,&fbc);
  if (!configv||(fbc<1)) return (GLXFBConfig){0};
  GLXFBConfig config=configv[0];
  XFree(configv);
  
  return config;
}

/* Create colormap and window.
 */
 
static int fmn_glx_init_window(struct fmn_hw_video *video,XVisualInfo *vi) {
  
  XSetWindowAttributes wattr={
    .background_pixel=0,
    .event_mask=
      StructureNotifyMask|
      KeyPressMask|KeyReleaseMask|
      FocusChangeMask|
    0,
  };
  wattr.colormap=XCreateColormap(VIDEO->dpy,RootWindow(VIDEO->dpy,vi->screen),vi->visual,AllocNone);
  
  if (!(VIDEO->win=XCreateWindow(
    VIDEO->dpy,RootWindow(VIDEO->dpy,VIDEO->screen),
    0,0,video->winw,video->winh,0,
    vi->depth,InputOutput,vi->visual,
    CWBackPixel|CWBorderPixel|CWColormap|CWEventMask,&wattr
  ))) return -1;
  
  return 0;
}

/* Set window title.
 */
 
static void fmn_glx_set_title(struct fmn_hw_video *video,const char *src) {
  
  // I've seen these properties in GNOME 2, unclear whether they might still be in play:
  XTextProperty prop={.value=(void*)src,.encoding=VIDEO->atom_STRING,.format=8,.nitems=0};
  while (prop.value[prop.nitems]) prop.nitems++;
  XSetWMName(VIDEO->dpy,VIDEO->win,&prop);
  XSetWMIconName(VIDEO->dpy,VIDEO->win,&prop);
  XSetTextProperty(VIDEO->dpy,VIDEO->win,&prop,VIDEO->atom__NET_WM_ICON_NAME);
    
  // This one becomes the window title and bottom-bar label, in GNOME 3:
  prop.encoding=VIDEO->atom_UTF8_STRING;
  XSetTextProperty(VIDEO->dpy,VIDEO->win,&prop,VIDEO->atom__NET_WM_NAME);
    
  // This daffy bullshit becomes the Alt-Tab text in GNOME 3:
  {
    char tmp[256];
    int len=prop.nitems+1+prop.nitems;
    if (len<sizeof(tmp)) {
      memcpy(tmp,prop.value,prop.nitems);
      tmp[prop.nitems]=0;
      memcpy(tmp+prop.nitems+1,prop.value,prop.nitems);
      tmp[prop.nitems+1+prop.nitems]=0;
      prop.value=tmp;
      prop.nitems=prop.nitems+1+prop.nitems;
      prop.encoding=VIDEO->atom_STRING;
      XSetTextProperty(VIDEO->dpy,VIDEO->win,&prop,VIDEO->atom_WM_CLASS);
    }
  }
}

/* Init.
 */
 
static int _glx_init(struct fmn_hw_video *video,const struct fmn_hw_video_params *params) {

  if (fmn_glx_init_display(video)<0) return -1;
  
  if (params) {
    video->winw=params->winw;
    video->winh=params->winh;
    video->fbw=params->fbw;
    video->fbh=params->fbh;
  }
  if ((video->fbw<1)||(video->fbh<1)) {
    video->fbw=160;
    video->fbh=90;
  }
  if ((video->winw<1)||(video->winh<1)) {
    int monw,monh;
    fmn_glx_estimate_monitor_size(&monw,&monh,video);
    video->winw=(monw*3)>>2;
    video->winh=(monh*3)>>2;
  }
  
  GLXFBConfig fbconfig=fmn_glx_get_fbconfig(video);
  XVisualInfo *vi=glXGetVisualFromFBConfig(VIDEO->dpy,fbconfig);
  if (!vi) return -1;
  int err=fmn_glx_init_window(video,vi);
  XFree(vi);
  if (err<0) return -1;
  
  if (params&&params->fullscreen) {
    XChangeProperty(
      VIDEO->dpy,VIDEO->win,
      VIDEO->atom__NET_WM_STATE,
      XA_ATOM,32,PropModeReplace,
      (unsigned char*)&VIDEO->atom__NET_WM_STATE_FULLSCREEN,1
    );
    video->fullscreen=1;
  }
  
  if (params&&params->title&&params->title[0]) {
    fmn_glx_set_title(video,params->title);
  }

  if (params&&params->iconrgba) {
    fmn_glx_set_icon(video,params->iconrgba,params->iconw,params->iconh);
  }
  
  XMapWindow(VIDEO->dpy,VIDEO->win);
  if (!(VIDEO->ctx=glXCreateNewContext(VIDEO->dpy,fbconfig,GLX_RGBA_TYPE,0,1))) return -1;
  glXMakeCurrent(VIDEO->dpy,VIDEO->win,VIDEO->ctx);
  XSync(VIDEO->dpy,0);
  XSetWMProtocols(VIDEO->dpy,VIDEO->win,&VIDEO->atom_WM_DELETE_WINDOW,1);
  
  // Hide cursor.
  XColor color;
  Pixmap pixmap=XCreateBitmapFromData(VIDEO->dpy,VIDEO->win,"\0\0\0\0\0\0\0\0",1,1);
  Cursor cursor=XCreatePixmapCursor(VIDEO->dpy,pixmap,pixmap,&color,&color,0,0);
  XDefineCursor(VIDEO->dpy,VIDEO->win,cursor);
  XFreeCursor(VIDEO->dpy,cursor);
  XFreePixmap(VIDEO->dpy,pixmap);
  
  video->fbfmt=FMN_IMAGE_FMT_RGBA;
  if (params) video->fbfmt=params->fbfmt;
  if (!(VIDEO->fb=fmn_image_new_alloc(video->fbfmt,video->fbw,video->fbh))) return -1;
  glGenTextures(1,&VIDEO->fbtexid);
  if (!VIDEO->fbtexid) {
    glGenTextures(1,&VIDEO->fbtexid);
    if (!VIDEO->fbtexid) return -1;
  }
  glBindTexture(GL_TEXTURE_2D,VIDEO->fbtexid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  VIDEO->dstdirty=1;
  
  return 0;
}

/* Set fullscreen.
 */
 
static void _glx_set_fullscreen(struct fmn_hw_video *video,int fullscreen) {
  XEvent evt={
    .xclient={
      .type=ClientMessage,
      .message_type=VIDEO->atom__NET_WM_STATE,
      .send_event=1,
      .format=32,
      .window=VIDEO->win,
      .data={.l={
        fullscreen,
        VIDEO->atom__NET_WM_STATE_FULLSCREEN,
      }},
    }
  };
  XSendEvent(VIDEO->dpy,RootWindow(VIDEO->dpy,VIDEO->screen),0,SubstructureNotifyMask|SubstructureRedirectMask,&evt);
  XFlush(VIDEO->dpy);
  video->fullscreen=fullscreen;
}

/* Suppress screensaver.
 */
 
static void _glx_suppress_screensaver(struct fmn_hw_video *video) {
  if (VIDEO->screensaver_suppressed) return;
  XForceScreenSaver(VIDEO->dpy,ScreenSaverReset);
  VIDEO->screensaver_suppressed=1;
}

/* Allocate (fbscratch) if needed, and convert (fb) into it.
 * Both (fbscratch) and (fb->v) are presumed contiguous.
 */
 
static int fmn_glx_fbcvt_y8_y2(struct fmn_hw_video *video) {
  if (!VIDEO->fbscratch) {
    if (!(VIDEO->fbscratch=malloc(VIDEO->fb->w*VIDEO->fb->h))) return -1;
  }
  uint8_t *dstp=VIDEO->fbscratch;
  const uint8_t *srcp=VIDEO->fb->v;
  uint8_t srcshift=6;
  int i=VIDEO->fb->w*VIDEO->fb->h;
  for (;i-->0;dstp++) {
    switch (((*srcp)>>srcshift)&3) {
      case 0: *dstp=0x00; break;
      case 1: *dstp=0x55; break;
      case 2: *dstp=0xaa; break;
      case 3: *dstp=0xff; break;
    }
    if (srcshift) srcshift-=2;
    else { srcshift=6; srcp++; }
  }
  return 0;
}

/* Swap buffers.
 */
 
static struct fmn_image *_glx_begin(struct fmn_hw_video *video) {
  return VIDEO->fb;
}

static void _glx_end(struct fmn_hw_video *video,struct fmn_image *fb) {
  if (fb!=VIDEO->fb) return;
  
  if (VIDEO->dstdirty) {
    VIDEO->dstdirty=0;
    const int fudgepx=10; // If it ends up this close to the window, fill it and allow distortion.
    int wforh=(video->winh*video->fbw)/video->fbh;
    if (wforh<=video->winw) {
      VIDEO->dstw=wforh;
      VIDEO->dsth=video->winh;
      if (VIDEO->dstw>=video->winw-fudgepx) VIDEO->dstw=video->winw;
    } else {
      VIDEO->dstw=video->winw;
      VIDEO->dsth=(video->winw*video->fbh)/video->fbw;
      if (VIDEO->dsth>=video->winh-fudgepx) VIDEO->dsth=video->winh;
    }
    VIDEO->dstx=(video->winw>>1)-(VIDEO->dstw>>1);
    VIDEO->dsty=(video->winh>>1)-(VIDEO->dsth>>1);
  }
  
  glBindTexture(GL_TEXTURE_2D,VIDEO->fbtexid);
  switch (fb->fmt) {
    case FMN_IMAGE_FMT_RGBA: glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,fb->w,fb->h,0,GL_RGBA,GL_UNSIGNED_BYTE,fb->v); break;
    case FMN_IMAGE_FMT_Y8: glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,fb->w,fb->h,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,fb->v); break;
    case FMN_IMAGE_FMT_Y2: {
        if (fmn_glx_fbcvt_y8_y2(video)<0) return;
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,fb->w,fb->h,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,VIDEO->fbscratch);
      } break;
    default: return;
  }
  
  if ((VIDEO->dstw<video->winw)||(VIDEO->dsth<video->winh)) {
    glViewport(0,0,video->winw,video->winh);
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  glViewport(VIDEO->dstx,VIDEO->dsty,VIDEO->dstw,VIDEO->dsth);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2i(0,0); glVertex2i(-1, 1);
    glTexCoord2i(0,1); glVertex2i(-1,-1);
    glTexCoord2i(1,0); glVertex2i( 1, 1);
    glTexCoord2i(1,1); glVertex2i( 1,-1);
  glEnd();

  glXSwapBuffers(VIDEO->dpy,VIDEO->win);
  VIDEO->screensaver_suppressed=0;
}

/* Type definition.
 */
 
const struct fmn_hw_video_type fmn_hw_video_type_glx={
  .name="glx",
  .desc="X11 with OpenGL, for Linux.",
  .objlen=sizeof(struct fmn_hw_video_glx),
  .provides_system_keyboard=1,
  .del=_glx_del,
  .init=_glx_init,
  .update=fmn_glx_update,
  .begin=_glx_begin,
  .end=_glx_end,
  .set_fullscreen=_glx_set_fullscreen,
  .suppress_screensaver=_glx_suppress_screensaver,
};
