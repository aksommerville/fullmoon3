#ifndef FMN_EDITOR_INTERNAL_H
#define FMN_EDITOR_INTERNAL_H

#include "tool/common/fmn_resmgr.h"
#include "opt/http/http.h"
#include "opt/fs/fmn_fs.h"
#include "opt/serial/fmn_serial.h"
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

extern struct fmn_editor {
  struct http_context *http;
  volatile int sigc;
} fmn_editor;

int fmn_editor_GET_restoc(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp);
int fmn_editor_GET_resall(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp);
int fmn_editor_serve_static(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp);

#endif
