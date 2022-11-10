#ifndef FMN_WEBLOCAL_INTERNAL_H
#define FMN_WEBLOCAL_INTERNAL_H

#include "opt/http/http.h"
#include "opt/fs/fmn_fs.h"
#include "opt/serial/fmn_serial.h"
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

extern struct fmn_weblocal {
  struct http_context *http;
  volatile int sigc;
  char **htdocsv;
  int htdocsc,htdocsa;
  char *host;
  int port;
} fmn_weblocal;

int fmn_weblocal_serve_static(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp);

#endif
