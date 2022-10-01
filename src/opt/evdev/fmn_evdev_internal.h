#ifndef FMN_EVDEV_INTERNAL_H
#define FMN_EVDEV_INTERNAL_H

#include "opt/hw/fmn_hw.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/poll.h>
#include <sys/inotify.h>
#include <linux/input.h>
#ifndef KEY_MISC
  #include <linux/input-event-codes.h>
#endif

#define FMN_EVDEV_DIR "/dev/input/"
#define FMN_EVDEV_NAME_LIMIT 64

struct fmn_hw_input_evdev {
  struct fmn_hw_input hdr;
  int infd;
  int scan;
  struct fmn_evdev_device {
    int fd;
    int kid;
    int devid;
    char name[FMN_EVDEV_NAME_LIMIT]; // always terminated
    int vid,pid;
  } *devicev;
  int devicec,devicea;
  struct pollfd *pollfdv;
  int pollfdc,pollfda;
};

#define INPUT ((struct fmn_hw_input_evdev*)input)

void fmn_evdev_device_cleanup(struct fmn_evdev_device *device);

struct fmn_evdev_device *fmn_evdev_get_device_by_fd(const struct fmn_hw_input *input,int fd);
struct fmn_evdev_device *fmn_evdev_get_device_by_kid(const struct fmn_hw_input *input,int kid);
struct fmn_evdev_device *fmn_evdev_get_device_by_devid(const struct fmn_hw_input *input,int devid);

int fmn_evdev_usage_from_code(uint8_t type,uint16_t code);

#endif
