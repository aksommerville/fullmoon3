#include "fmn_evdev_internal.h"

/* Cleanup.
 */
 
void fmn_evdev_device_cleanup(struct fmn_evdev_device *device) {
  if (device->fd>=0) close(device->fd);
}
 
static void _evdev_del(struct fmn_hw_input *input) {
  if (INPUT->infd>=0) close(INPUT->infd);
  if (INPUT->devicev) {
    while (INPUT->devicec-->0) fmn_evdev_device_cleanup(INPUT->devicev+INPUT->devicec);
    free(INPUT->devicev);
  }
  if (INPUT->pollfdv) free(INPUT->pollfdv);
}

/* Init.
 */
 
static int _evdev_init(struct fmn_hw_input *input) {
  if ((INPUT->infd=inotify_init())>=0) {
    inotify_add_watch(INPUT->infd,FMN_EVDEV_DIR,IN_ATTRIB|IN_CREATE|IN_MOVED_TO);
  }
  INPUT->scan=1;
  return 0;
}

/* Find device in our list.
 * The list is not sorted.
 */
 
struct fmn_evdev_device *fmn_evdev_get_device_by_fd(const struct fmn_hw_input *input,int fd) {
  struct fmn_evdev_device *device=INPUT->devicev;
  int i=INPUT->devicec;
  for (;i-->0;device++) {
    if (device->fd==fd) return device;
  }
  return 0;
}

struct fmn_evdev_device *fmn_evdev_get_device_by_kid(const struct fmn_hw_input *input,int kid) {
  struct fmn_evdev_device *device=INPUT->devicev;
  int i=INPUT->devicec;
  for (;i-->0;device++) {
    if (device->kid==kid) return device;
  }
  return 0;
}

struct fmn_evdev_device *fmn_evdev_get_device_by_devid(const struct fmn_hw_input *input,int devid) {
  struct fmn_evdev_device *device=INPUT->devicev;
  int i=INPUT->devicec;
  for (;i-->0;device++) {
    if (device->devid==devid) return device;
  }
  return 0;
}

/* Examine one file in the device directory.
 * Fails only for the most egregious errors eg out of memory.
 */
 
static int fmn_evdev_consider_file(struct fmn_hw_input *input,const char *base,int basec) {

  // Confirm it's evdev, extract kid, confirm we don't have it already.
  if (!base) return 0;
  if (basec<0) { basec=0; while (base[basec]) basec++; }
  if ((basec<6)||memcmp(base,"event",5)) return 0;
  int kid=0,i=5;
  for (;i<basec;i++) {
    if ((base[i]<'0')||(base[i]>'9')) return 0;
    kid*=10;
    kid+=base[i]-'0';
    if (kid>10000) return 0; // i mean, come on
  }
  if (fmn_evdev_get_device_by_kid(input,kid)) return 0;
  
  // Apportion room in devicev.
  if (INPUT->devicec>=INPUT->devicea) {
    int na=INPUT->devicea+16;
    if (na>INT_MAX/sizeof(struct fmn_evdev_device)) return -1;
    void *nv=realloc(INPUT->devicev,sizeof(struct fmn_evdev_device)*na);
    if (!nv) return -1;
    INPUT->devicev=nv;
    INPUT->devicea=na;
  }
  
  // Open the file and confirm it responds to EVIOCGVERSION.
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%s%.*s",FMN_EVDEV_DIR,basec,base);
  if ((pathc<1)||(pathc>=sizeof(path))) return 0;
  int fd=open(path,O_RDONLY);
  if (fd<0) return 0;
  int version;
  if (ioctl(fd,EVIOCGVERSION,&version)<0) {
    close(fd);
    return 0;
  }
  
  // Add to device list.
  struct fmn_evdev_device *device=INPUT->devicev+INPUT->devicec++;
  memset(device,0,sizeof(struct fmn_evdev_device));
  device->fd=fd;
  device->kid=kid;
  if ((device->devid=fmn_hw_devid_next())<0) { // ay yi yi
    fmn_evdev_device_cleanup(device);
    INPUT->devicec--;
    return 0;
  }
  
  // Acquire IDs.
  struct input_id id={0};
  ioctl(fd,EVIOCGID,&id);
  device->vid=id.vendor;
  device->pid=id.product;
  ioctl(fd,EVIOCGNAME(sizeof(device->name)),device->name);
  device->name[sizeof(device->name)-1]=0;
  char *p=device->name;
  for (i=sizeof(device->name);i-->0;p++) {
    if ((*p<0x20)||(*p>0x7e)) *p='?';
  }
  
  // Notify owner.
  if (input->delegate->connect) {
    input->delegate->connect(input,device->devid);
  }
  
  return 0;
}

/* Scan device directory.
 */
 
static int fmn_evdev_scan(struct fmn_hw_input *input) {
  DIR *dir=opendir(FMN_EVDEV_DIR);
  if (!dir) return -1;
  struct dirent *de;
  while (de=readdir(dir)) {
    if (de->d_type!=DT_CHR) continue;
    if (fmn_evdev_consider_file(input,de->d_name,-1)<0) {
      closedir(dir);
      return -1;
    }
  }
  closedir(dir);
  return 0;
}

/* Read from inotify.
 */
 
static int fmn_evdev_update_inotify(struct fmn_hw_input *input) {
  char buf[1024];
  int bufc=read(INPUT->infd,buf,sizeof(buf));
  if (bufc<=0) {
    fprintf(stderr,"evdev: Inotify connection lost. New input devices will not be detected.\n");
    close(INPUT->infd);
    INPUT->infd=-1;
    return 0;
  }
  int bufp=0;
  while (bufp<=bufc-sizeof(struct inotify_event)) {
    struct inotify_event *event=(struct inotify_event*)(buf+bufp);
    bufp+=sizeof(struct inotify_event);
    if (bufp>bufc-event->len) break;
    const char *base=buf+bufp;
    int basec=0;
    while ((basec<event->len)&&base[basec]) basec++;
    bufp+=event->len;
    if (!basec) continue;
    if (fmn_evdev_consider_file(input,base,basec)<0) return -1;
  }
  return 0;
}

/* Read from device.
 */
 
static int fmn_evdev_update_device(struct fmn_hw_input *input,int fd) {
  struct fmn_evdev_device *device=fmn_evdev_get_device_by_fd(input,fd);
  if (!device) return -1;
  struct input_event eventv[16];
  int eventc=read(fd,eventv,sizeof(eventv));
  if (eventc<=0) {
    int devid=device->devid;
    fmn_evdev_device_cleanup(device);
    INPUT->devicec--;
    int p=device-INPUT->devicev;
    memmove(device,device+1,sizeof(struct fmn_evdev_device)*(INPUT->devicec-p));
    if (input->delegate->disconnect) input->delegate->disconnect(input,devid);
    return 0;
  }
  if (input->delegate->button) {
    eventc/=sizeof(struct input_event);
    const struct input_event *event=eventv;
    for (;eventc-->0;event++) {
      if (event->type==EV_SYN) continue;
      input->delegate->button(input,device->devid,(event->type<<16)|event->code,event->value);
    }
  }
  return 0;
}

/* Rebuild pollfdv.
 */
 
static int fmn_evdev_rebuild_pollfdv(struct fmn_hw_input *input) {

  int na=((INPUT->infd>=0)?1:0)+INPUT->devicec;
  if (na>INPUT->pollfda) {
    na=(na+8)&~7;
    if (na>INT_MAX/sizeof(struct pollfd)) return -1;
    void *nv=realloc(INPUT->pollfdv,sizeof(struct pollfd)*na);
    if (!nv) return -1;
    INPUT->pollfdv=nv;
    INPUT->pollfda=na;
  }

  INPUT->pollfdc=0;
  if (INPUT->infd>=0) {
    struct pollfd *pollfd=INPUT->pollfdv+INPUT->pollfdc++;
    pollfd->fd=INPUT->infd;
    pollfd->events=POLLIN|POLLERR|POLLHUP;
    pollfd->revents=0;
  }
  struct fmn_evdev_device *device=INPUT->devicev;
  int i=INPUT->devicec;
  for (;i-->0;device++) {
    struct pollfd *pollfd=INPUT->pollfdv+INPUT->pollfdc++;
    pollfd->fd=device->fd;
    pollfd->events=POLLIN|POLLERR|POLLHUP;
    pollfd->revents=0;
  }
  
  return 0;
}

/* Update.
 */
 
static int _evdev_update(struct fmn_hw_input *input) {
  if (INPUT->scan) {
    INPUT->scan=0;
    if (fmn_evdev_scan(input)<0) return -1;
  }
  if (fmn_evdev_rebuild_pollfdv(input)<0) return -1;
  if (INPUT->pollfdc<1) return 0;
  int err=poll(INPUT->pollfdv,INPUT->pollfdc,0);
  if (err<=0) return 0;
  struct pollfd *pollfd=INPUT->pollfdv;
  int i=INPUT->pollfdc;
  for (;i-->0;pollfd++) {
    if (!pollfd->revents) continue;
    if (pollfd->fd==INPUT->infd) {
      if (fmn_evdev_update_inotify(input)<0) err=-1;
    } else {
      if (fmn_evdev_update_device(input,pollfd->fd)<0) err=-1;
    }
  }
  return err;
}

/* Get device IDs.
 */
 
static const char *_evdev_get_ids(int *vid,int *pid,struct fmn_hw_input *input,int devid) {
  struct fmn_evdev_device *device=fmn_evdev_get_device_by_devid(input,devid);
  if (!device) return 0;
  if (vid) *vid=device->vid;
  if (pid) *pid=device->pid;
  return device->name;
}

/* Enumerate device buttons.
 */
 
static int _evdev_enumerate(
  struct fmn_hw_input *input,
  int devid,
  int (*cb)(struct fmn_hw_input *input,int devid,int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  struct fmn_evdev_device *device=fmn_evdev_get_device_by_devid(input,devid);
  if (!device) return -1;
  int err;
  uint8_t bits[256]={0};
  int bytec=ioctl(device->fd,EVIOCGBIT(EV_KEY,sizeof(bits)),bits);
  int major=0;
  for (;major<bytec;major++) {
    if (!bits[major]) continue;
    int minor=0;
    uint8_t mask=1;
    for (;minor<8;minor++,mask<<=1) {
      if (!(bits[major]&mask)) continue;
      int code=(major<<3)|minor;
      int usage=0; //TODO Should we guess HID usage?
      if (err=cb(input,devid,(EV_KEY<<16)|code,usage,0,1,0,userdata)) return err;
    }
  }
  if ((bytec=ioctl(device->fd,EVIOCGBIT(EV_ABS,sizeof(bits)),bits))>0) {
    for (major=0;major<bytec;major++) {
      if (!bits[major]) continue;
      int minor=0;
      uint8_t mask=1;
      for (;minor<8;minor++,mask<<=1) {
        if (!(bits[major]&mask)) continue;
        struct input_absinfo info={0};
        int code=(major<<3)|minor;
        ioctl(device->fd,EVIOCGABS(code),&info);
        int usage=0; //TODO Should we guess HID usage?
        if (err=cb(input,devid,(EV_ABS<<16)|code,usage,info.minimum,info.maximum,info.value,userdata)) return err;
      }
    }
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_hw_input_type fmn_hw_input_type_evdev={
  .name="evdev",
  .desc="General input for Linux.",
  .objlen=sizeof(struct fmn_hw_input_evdev),
  .del=_evdev_del,
  .init=_evdev_init,
  .update=_evdev_update,
  .get_ids=_evdev_get_ids,
  .enumerate=_evdev_enumerate,
};
