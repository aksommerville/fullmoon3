#include "http_internal.h"

/* Delete.
 */
 
void http_listener_del(struct http_listener *listener) {
  if (!listener) return;
  if (listener->refc-->1) return;
  if (listener->method) free(listener->method);
  if (listener->path) free(listener->path);
  free(listener);
}

/* Retain.
 */
 
int http_listener_ref(struct http_listener *listener) {
  if (!listener) return -1;
  if (listener->refc<1) return -1;
  if (listener->refc==INT_MAX) return -1;
  listener->refc++;
  return 0;
}

/* New.
 */

struct http_listener *http_listener_new(
  int (*serve)(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp),
  void *userdata
) {
  if (!serve) return 0;
  struct http_listener *listener=calloc(1,sizeof(struct http_listener));
  if (!listener) return 0;
  listener->refc=1;
  listener->serve=serve;
  listener->userdata=userdata;
  return listener;
}

/* Trivial accessors.
 */

void *http_listener_get_userdata(const struct http_listener *listener) {
  if (!listener) return 0;
  return listener->userdata;
}

struct http_context *http_listener_get_context(const struct http_listener *listener) {
  if (!listener) return 0;
  return listener->context;
}

/* Set path.
 */

int http_listener_set_path(struct http_listener *listener,const char *path,int pathc) {
  if (!listener) return -1;
  if (!path) pathc=0; else if (pathc<0) { pathc=0; while (path[pathc]) pathc++; }
  
  // A path of all stars is equivalent to empty, and empty is a little simpler.
  int i=pathc,allstars=1;
  while (i-->0) if (path[i]!='*') { allstars=0; break; }
  if (allstars) pathc=0;
  
  char *nv=0;
  if (pathc) {
    if (!(nv=malloc(pathc+1))) return -1;
    memcpy(nv,path,pathc);
    nv[pathc]=0;
  }
  if (listener->path) free(listener->path);
  listener->path=nv;
  listener->pathc=pathc;
  return 0;
}

/* Set method.
 */
 
int http_listener_set_method(struct http_listener *listener,const char *method,int methodc) {
  if (!listener) return -1;
  if (!method) methodc=0; else if (methodc<0) { methodc=0; while (method[methodc]) methodc++; }
  char *nv=0;
  if (methodc) {
    if (!(nv=malloc(methodc+1))) return -1;
    memcpy(nv,method,methodc);
    nv[methodc]=0;
  }
  if (listener->method) free(listener->method);
  listener->method=nv;
  listener->methodc=methodc;
  return 0;
}

/* Match request.
 */
 
int http_listener_match_request(const struct http_listener *listener,const struct http_xfer *req) {
  if (!listener||!req) return 0;
  if (listener->methodc) {
    const char *q;
    int qc=http_xfer_get_method(&q,req);
    if ((qc!=listener->methodc)||fmn_memcasecmp(listener->method,q,qc)) return 0;
  }
  if (listener->pathc) {
    const char *q;
    int qc=http_xfer_get_path(&q,req);
    if (qc<1) return 0;
    if (!fmn_wildcard_match(listener->path,listener->pathc,q,qc)) return 0;
  }
  return 1;
}
