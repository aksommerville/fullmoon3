#ifndef HTTP_INTERNAL_H
#define HTTP_INTERNAL_H

#include "http.h"
#include "opt/serial/fmn_serial.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Context.
 ********************************************************/
 
struct http_context {
  int refc;
  struct http_listener **listenerv;
  int listenerc,listenera;
  struct http_conn **connv;
  int connc,conna;
  struct pollfd *pollfdv;
  int pollfdc,pollfda;
};

struct http_conn *http_context_find_conn_by_fd(const struct http_context *context,int fd);

// For now this just creates a new one. Might add a pool later.
struct http_xfer *http_context_generate_xfer(struct http_context *context);

/* Listener.
 *********************************************************/
 
struct http_listener {
  int refc;
  int (*serve)(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp);
  void *userdata;
  char *method; int methodc;
  char *path; int pathc;
  struct http_context *context;
};

/* Connection.
 *********************************************************/
 
struct http_conn {
  int refc;
  int fd;
  int role;
  int defunct; // nonzero to drop when convenient (also happens if fd<0)
  char *wbuf;
  int wbufp,wbufc,wbufa;
  char *rbuf;
  int rbufp,rbufc,rbufa;
  void *saddr;
  int saddrc;
  struct http_xfer *xfer; // for streams, if an inbound transfer is in progress.
};

/* Reading returns 0 if the basic read fails, typically due to closed remote end, >0 success, or <0 for unlikely local errors.
 * Accept creates a new STREAM connection on success.
 */
int http_conn_update_read(struct http_conn *conn);
int http_conn_update_write(struct http_conn *conn);
struct http_conn *http_conn_update_accept(struct http_conn *conn);

int http_conn_set_xfer(struct http_conn *conn,struct http_xfer *xfer);

int http_conn_wbuf_append(struct http_conn *conn,const void *src,int srcc);
int http_conn_wbuf_appendf(struct http_conn *conn,const char *fmt,...);

/* Transfer.
 ***********************************************************/
 
#define HTTP_XFER_STAGE_IDLE       0 /* Awaiting data. */
#define HTTP_XFER_STAGE_LINE1      1 /* Receiving Status-Line or Request-Line. */
#define HTTP_XFER_STAGE_HEADER     2 /* Receiving headers. */
#define HTTP_XFER_STAGE_BODY       3 /* Receiving unchunked body. */
#define HTTP_XFER_STAGE_CHUNKLEN   4 /* Receiving chunk length. */
#define HTTP_XFER_STAGE_CHUNK      5 /* Receiving chunk content. */
 
struct http_xfer {
  int refc;
  int role;
  int stage;
  int expect; // <0 if the next block of input should be newline-terminated, otherwise expect this much.
  char *line1;
  int line1c;
  struct http_header {
    char *k,*v;
    int kc,vc;
  } *headerv;
  int headerc,headera;
  void *body;
  int bodyc,bodya;
  struct http_conn *conn; // WEAK
  struct http_context *context; // WEAK
};

int http_xfer_encode(struct http_conn *conn,struct http_xfer *xfer);

#endif
