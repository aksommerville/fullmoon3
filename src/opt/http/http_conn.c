#include "http_internal.h"
#include <stdarg.h>

/* Delete.
 */
 
void http_conn_del(struct http_conn *conn) {
  if (!conn) return;
  if (conn->refc-->1) return;
  if (conn->fd>=0) close(conn->fd);
  if (conn->wbuf) free(conn->wbuf);
  if (conn->rbuf) free(conn->rbuf);
  if (conn->saddr) free(conn->saddr);
  if (conn->xfer) {
    conn->xfer->conn=0;
    http_xfer_del(conn->xfer);
  }
  free(conn);
}

/* Retain.
 */
 
int http_conn_ref(struct http_conn *conn) {
  if (!conn) return -1;
  if (conn->refc<1) return -1;
  if (conn->refc==INT_MAX) return -1;
  conn->refc++;
  return 0;
}

/* New.
 */

struct http_conn *http_conn_new() {
  struct http_conn *conn=calloc(1,sizeof(struct http_conn));
  if (!conn) return 0;
  conn->refc=1;
  conn->fd=-1;
  return conn;
}

/* Trivial accessors.
 */
 
int http_conn_set_xfer(struct http_conn *conn,struct http_xfer *xfer) {
  if (!conn) return -1;
  if (conn->xfer==xfer) return 0;
  if (xfer&&(http_xfer_ref(xfer)<0)) return -1;
  if (conn->xfer) {
    conn->xfer->conn=0;
    http_xfer_del(conn->xfer);
  }
  conn->xfer=xfer;
  xfer->conn=conn;
  return 0;
}

/* Setup TCP server.
 */

int http_conn_listen_tcp(struct http_conn *conn,const char *host,int hostc,int port) {
  if (!conn) return -1;
  if (conn->fd>=0) return -1;
  if (!host) hostc=0; else if (hostc<0) { hostc=0; while (host[hostc]) hostc++; }
  char zhost[256];
  if (hostc>=sizeof(zhost)) return -1;
  memcpy(zhost,host,hostc);
  zhost[hostc]=0;
  char zport[32];
  snprintf(zport,sizeof(zport),"%d",port);
  
  struct addrinfo hints={
    .ai_family=AF_UNSPEC,
    .ai_socktype=SOCK_STREAM,
    .ai_flags=AI_NUMERICSERV|AI_ADDRCONFIG, // AI_PASSIVE and null zhost for INADDR_ANY
  };
  struct addrinfo *ai=0;
  if (getaddrinfo(zhost,zport,&hints,&ai)) return -1;
  
  if ((conn->fd=socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol))<0) {
    freeaddrinfo(ai);
    return -1;
  }
  
  int one=1;
  setsockopt(conn->fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
  
  if (bind(conn->fd,ai->ai_addr,ai->ai_addrlen)<0) {
    freeaddrinfo(ai);
    close(conn->fd);
    conn->fd=-1;
    return -1;
  }
  http_conn_set_saddr(conn,ai->ai_addr,ai->ai_addrlen);
  freeaddrinfo(ai);
  
  if (listen(conn->fd,10)<0) {
    close(conn->fd);
    conn->fd=-1;
    return -1;
  }
  
  conn->role=HTTP_CONN_ROLE_SERVER;
  return 0;
}

/* Setup explicitly.
 */
 
int http_conn_set_fd(struct http_conn *conn,int fd,int role) {
  if (!conn) return -1;
  if (fd<0) return -1;
  if (conn->fd>=0) return -1;
  conn->fd=fd;
  conn->role=role;
  return 0;
}

/* Set sockaddr.
 */
 
int http_conn_set_saddr(struct http_conn *conn,const void *saddr,int saddrc) {
  if (!conn) return -1;
  if (!saddr||(saddrc<0)) saddrc=0;
  char *nv=0;
  if (saddrc) {
    if (!(nv=malloc(saddrc))) return -1;
    memcpy(nv,saddr,saddrc);
  }
  if (conn->saddr) free(conn->saddr);
  conn->saddr=nv;
  conn->saddrc=saddrc;
  return 0;
}

/* Close.
 */

void http_conn_close(struct http_conn *conn) {
  if (!conn) return;
  if (conn->fd<0) return;
  close(conn->fd);
  conn->fd=-1;
  conn->defunct=1;
}

/* Grow buffers.
 */
 
int http_conn_rbuf_require(struct http_conn *conn,int addc) {
  if (addc<1) return 0;
  if (conn->rbufp+conn->rbufc<=conn->rbufa-addc) return 0;
  if (conn->rbufp) {
    memmove(conn->rbuf,conn->rbuf+conn->rbufp,conn->rbufc);
    conn->rbufp=0;
    if (conn->rbufc<=conn->rbufa-addc) return 0;
  }
  if (conn->rbufc>INT_MAX-addc) return -1;
  int na=conn->rbufc+addc;
  if (na<INT_MAX-1024) na=(na+1024)&~1023;
  void *nv=realloc(conn->rbuf,na);
  if (!nv) return -1;
  conn->rbuf=nv;
  conn->rbufa=na;
  return 0;
}

void *http_conn_wbuf_require(struct http_conn *conn,int addc) {
  if (addc<1) return conn->wbuf+conn->wbufp+conn->wbufc;
  if (conn->wbufp+conn->wbufc<=conn->wbufa-addc) return conn->wbuf+conn->wbufp+conn->wbufc;
  if (conn->wbufp) {
    memmove(conn->wbuf,conn->wbuf+conn->wbufp,conn->wbufc);
    conn->wbufp=0;
    if (conn->wbufc<=conn->wbufa-addc) return conn->wbuf+conn->wbufc;
  }
  if (conn->wbufc>INT_MAX-addc) return 0;
  int na=conn->wbufc+addc;
  if (na<INT_MAX-1024) na=(na+1024)&~1023;
  void *nv=realloc(conn->wbuf,na);
  if (!nv) return 0;
  conn->wbuf=nv;
  conn->wbufa=na;
  return conn->wbuf+conn->wbufc;
}

/* Append to wbuf.
 */
 
int http_conn_wbuf_append(struct http_conn *conn,const void *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (((char*)src)[srcc]) srcc++; }
  void *dst=http_conn_wbuf_require(conn,srcc);
  if (!dst) return -1;
  memcpy(dst,src,srcc);
  conn->wbufc+=srcc;
  return 0;
}

int http_conn_wbuf_appendf(struct http_conn *conn,const char *fmt,...) {
  if (!fmt||!fmt[0]) return 0;
  while (1) {
    va_list vargs;
    va_start(vargs,fmt);
    int a=conn->wbufa-conn->wbufc-conn->wbufp;
    int err=vsnprintf(conn->wbuf+conn->wbufp+conn->wbufc,a,fmt,vargs);
    if ((err<0)||(err==INT_MAX)) return -1;
    if (err<a) { // sic < not <=
      conn->wbufc+=err;
      return 0;
    }
    if (!http_conn_wbuf_require(conn,err+1)) return -1;
  }
}

/* Update, read.
 */
 
int http_conn_update_read(struct http_conn *conn) {
  if (http_conn_rbuf_require(conn,1)<0) return -1;
  int err=read(conn->fd,conn->rbuf+conn->rbufp+conn->rbufc,conn->rbufa-conn->rbufc-conn->rbufp);
  if (err<=0) return err;
  conn->rbufc+=err;
  return 1;
}

/* Update, write.
 */
 
int http_conn_update_write(struct http_conn *conn) {
  int err=write(conn->fd,conn->wbuf+conn->wbufp,conn->wbufc);
  if (err<=0) return err;
  conn->wbufp+=err;
  conn->wbufc-=err;
  return 1;
}

/* Update, accept.
 */
 
struct http_conn *http_conn_update_accept(struct http_conn *conn) {
  struct sockaddr_in saddr={0};
  socklen_t saddrc=sizeof(saddr);
  int fd=accept(conn->fd,(struct sockaddr*)&saddr,&saddrc);
  if (fd<0) return 0;
  struct http_conn *stream=http_conn_new();
  if (!stream) {
    close(fd);
    return 0;
  }
  if (http_conn_set_fd(stream,fd,HTTP_CONN_ROLE_STREAM)<0) {
    close(fd);
    return 0;
  }
  http_conn_set_saddr(stream,&saddr,saddrc);
  return stream;
}

/* Describe conn.
 */
 
int http_conn_describe(char *dst,int dsta,const struct http_conn *conn) {
  if (!dst||(dsta<0)) dsta=0;
  int dstc=0;
  if (conn) {
    if (conn->saddrc>=sizeof(struct sockaddr)) switch (((struct sockaddr*)conn->saddr)->sa_family) {
      case AF_INET: {
          const struct sockaddr_in *sin=conn->saddr;
          const uint8_t *b=(uint8_t*)&sin->sin_addr;
          int port=ntohs(sin->sin_port);
          dstc=snprintf(dst,dsta,"%d.%d.%d.%d:%d",b[0],b[1],b[2],b[3],port);
        } break;
      case AF_INET6: {
          //TODO
        } break;
    }
    if (dstc<=0) {
      dstc=0;
      if (conn->fd>=0) dstc=snprintf(dst,dsta,"fd=%d",conn->fd);
    }
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}
  
