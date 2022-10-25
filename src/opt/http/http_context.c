#include "http_internal.h"

/* Delete.
 */
 
void http_context_del(struct http_context *context) {
  if (!context) return;
  if (context->refc-->1) return;
  
  if (context->listenerv) {
    while (context->listenerc-->0) {
      struct http_listener *listener=context->listenerv[context->listenerc];
      listener->context=0;
      http_listener_del(listener);
    }
    free(context->listenerv);
  }
  
  if (context->connv) {
    while (context->connc-->0) {
      struct http_conn *conn=context->connv[context->connc];
      http_conn_del(conn);
    }
    free(context->connv);
  }
  
  if (context->pollfdv) free(context->pollfdv);
  
  free(context);
}

/* Retain.
 */
 
int http_context_ref(struct http_context *context) {
  if (!context) return -1;
  if (context->refc<1) return -1;
  if (context->refc==INT_MAX) return -1;
  context->refc++;
  return 0;
}

/* New.
 */
 
struct http_context *http_context_new() {
  struct http_context *context=calloc(1,sizeof(struct http_context));
  if (!context) return 0;
  context->refc=1;
  return context;
}

/* Describe servers for logging.
 */
 
int http_context_describe_servers(char *dst,int dsta,const struct http_context *context) {
  if (!dst||(dsta<1)) return 0;
  int dstc=0,i=0;
  for (;i<context->connc;i++) {
    struct http_conn *conn=context->connv[i];
    if (conn->role!=HTTP_CONN_ROLE_SERVER) continue;
    if (dstc) {
      if (dstc<dsta) dst[dstc]=',';
      dstc++;
    }
    int err=http_conn_describe(dst+dstc,dsta-dstc,conn);
    if (err>=0) dstc+=err;
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* New TCP server.
 */

int http_context_serve_tcp(
  struct http_context *context,
  const char *host,int hostc,
  int port
) {
  if (!context) return -1;
  struct http_conn *conn=http_conn_new();
  if (!conn) return -1;
  if (http_conn_listen_tcp(conn,host,hostc,port)<0) {
    http_conn_del(conn);
    return -1;
  }
  if (http_context_add_conn(context,conn)<0) {
    http_conn_del(conn);
    return -1;
  }
  return 0;
}

/* Drop any conn no longer capable of updating.
 */
 
static void http_context_drop_defunct_conns(struct http_context *context) {
  int i=context->connc;
  while (i-->0) {
    struct http_conn *conn=context->connv[i];
    
    // If a STREAM connection has no xfer or outgoing content, it's defunct.
    if ((conn->fd>=0)&&!conn->defunct) {
      if ((conn->role==HTTP_CONN_ROLE_STREAM)&&!conn->xfer&&!conn->wbufc) {
        fprintf(stderr,"%s:%d: conn %p no activity, defuncting\n",__FILE__,__LINE__,conn);
        conn->defunct=1;
      }
    }
    
    if ((conn->fd<0)||conn->defunct) {
      context->connc--;
      memmove(context->connv+i,context->connv+i+1,sizeof(void*)*(context->connc-i));
      http_conn_del(conn);
    }
  }
}

/* Rebuild pollfdv.
 */
 
static int http_context_rebuild_pollfdv(struct http_context *context) {
  
  // We happen to know that there will be exactly as many pollfd as conn, so allocation is easy.
  // This won't necessarily be true in the future.
  if (context->connc>context->pollfda) {
    int na=(context->connc+8)&~7;
    if (na>INT_MAX/sizeof(struct pollfd)) return -1;
    void *nv=realloc(context->pollfdv,sizeof(struct pollfd)*na);
    if (!nv) return -1;
    context->pollfdv=nv;
    context->pollfda=na;
  }
  
  context->pollfdc=0;
  int i=context->connc;
  while (i-->0) {
    struct http_conn *conn=context->connv[i];
    struct pollfd *pollfd=context->pollfdv+context->pollfdc++;
    memset(pollfd,0,sizeof(struct pollfd));
    pollfd->fd=conn->fd;
    pollfd->events=POLLERR|POLLHUP;
    if (conn->wbufc) {
      pollfd->events|=POLLOUT;
    } else {
      pollfd->events|=POLLIN;
    }
  }
  
  return 0;
}

/* Update one file.
 * Fail only on hard errors, it will end the program.
 */
 
static int http_context_update_fd(struct http_context *context,int fd) {
  struct http_conn *conn=http_context_find_conn_by_fd(context,fd);
  if (!conn) return 0;
  
  if (conn->wbufc) {
    if (http_conn_update_write(conn)<=0) {
      fprintf(stderr,"Error writing to socket, will close connection.\n");
      conn->defunct=1;
    }
  
  } else if (conn->role==HTTP_CONN_ROLE_SERVER) {
    struct http_conn *stream=http_conn_update_accept(conn);
    if (stream) {
      if (http_context_add_conn(context,stream)<0) {
        http_conn_del(stream);
        return -1;
      }
      struct http_xfer *xfer=http_context_generate_xfer(context);
      if (!xfer||(http_conn_set_xfer(stream,xfer)<0)) {
        http_xfer_del(xfer);
        return -1;
      }
      xfer->context=context;
      http_xfer_del(xfer);
    } else {
      fprintf(stderr,"Error updating server socket, will close connection.\n");
      conn->defunct=1;
    }
  
  } else {
    int err=http_conn_update_read(conn);
    if (err<0) {
      fprintf(stderr,"Error reading from socket, will close connection.\n");
      conn->defunct=1;
    } else if (!err) {
      // Failure to read is perfectly normal, eg remote end closed. Don't log it.
      conn->defunct=1;
    } else {
      while (conn->xfer&&conn->rbufc) {
        int err=http_xfer_receive(conn->xfer,conn->rbuf+conn->rbufp,conn->rbufc);
        if (err<0) {
          fprintf(stderr,"Error processing %d bytes of incoming HTTP. Will close connection.\n",conn->rbufc);
          conn->defunct=1;
          break;
        }
        if (!err) break;
        conn->rbufp+=err;
        conn->rbufc-=err;
      }
    }
  }
  return 0;
}

/* Update.
 */

int http_context_update(struct http_context *context,int to_ms) {
  if (!context) return 0;
  http_context_drop_defunct_conns(context);
  if (http_context_rebuild_pollfdv(context)<0) return -1;
  if (context->pollfdc<1) {
    if (to_ms>0) usleep(to_ms/1000);
    return 0;
  }
  int err=poll(context->pollfdv,context->pollfdc,to_ms);
  if (err<=0) return 0;
  struct pollfd *pollfd=context->pollfdv;
  int i=context->pollfdc;
  for (;i-->0;pollfd++) {
    if (!pollfd->revents) continue;
    if (http_context_update_fd(context,pollfd->fd)<0) return -1;
  }
  return 1;
}

/* Add connection.
 */
 
int http_context_add_conn(struct http_context *context,struct http_conn *conn) {
  if (!context||!conn) return -1;
  if (context->connc>=context->conna) {
    int na=context->conna+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(context->connv,sizeof(void*)*na);
    if (!nv) return -1;
    context->connv=nv;
    context->conna=na;
  }
  if (http_conn_ref(conn)<0) return -1;
  context->connv[context->connc++]=conn;
  return 0;
}

/* Add listener.
 */

int http_context_add_listener(struct http_context *context,struct http_listener *listener) {
  if (!context||!listener) return -1;
  if (context->listenerc>=context->listenera) {
    int na=context->listenera+32;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(context->listenerv,sizeof(void*)*na);
    if (!nv) return -1;
    context->listenerv=nv;
    context->listenera=na;
  }
  if (http_listener_ref(listener)<0) return -1;
  context->listenerv[context->listenerc++]=listener;
  listener->context=context;
  return 0;
}

/* Spawn listener.
 */

struct http_listener *http_context_spawn_listener(
  struct http_context *context,
  const char *method,const char *path,
  int (*serve)(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp),
  void *userdata
) {
  struct http_listener *listener=http_listener_new(serve,userdata);
  if (!listener) return 0;
  if (
    (http_listener_set_method(listener,method,-1)<0)||
    (http_listener_set_path(listener,path,-1)<0)||
    (http_context_add_listener(context,listener)<0)
  ) {
    http_listener_del(listener);
    return 0;
  }
  http_listener_del(listener);
  return listener;
}

/* Search children.
 */
 
struct http_conn *http_context_find_conn_by_fd(const struct http_context *context,int fd) {
  if (!context) return 0;
  int i=context->connc;
  while (i-->0) {
    struct http_conn *conn=context->connv[i];
    if (conn->fd==fd) return conn;
  }
  return 0;
}

struct http_listener *http_context_find_listener_for_request(const struct http_context *context,const struct http_xfer *req) {
  if (!context||!req) return 0;
  int i=0; for (;i<context->listenerc;i++) {
    struct http_listener *listener=context->listenerv[i];
    if (http_listener_match_request(listener,req)) return listener;
  }
  return 0;
}

/* New xfer or pull from pool.
 */
 
struct http_xfer *http_context_generate_xfer(struct http_context *context) {
  struct http_xfer *xfer=http_xfer_new(HTTP_XFER_ROLE_SERVER_REQUEST);
  if (!xfer) return 0;
  xfer->context=context;
  return xfer;
}
