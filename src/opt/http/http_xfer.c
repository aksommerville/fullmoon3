#include "http_internal.h"
#include <stdarg.h>

/* Delete.
 */
 
static void http_header_cleanup(struct http_header *header) {
  if (header->k) free(header->k);
  if (header->v) free(header->v);
}
 
void http_xfer_del(struct http_xfer *xfer) {
  if (!xfer) return;
  if (xfer->refc-->1) return;
  
  if (xfer->line1) free(xfer->line1);
  if (xfer->headerv) {
    while (xfer->headerc-->0) http_header_cleanup(xfer->headerv+xfer->headerc);
    free(xfer->headerv);
  }
  if (xfer->body) free(xfer->body);
  
  free(xfer);
}

/* Retain.
 */
 
int http_xfer_ref(struct http_xfer *xfer) {
  if (!xfer) return -1;
  if (xfer->refc<1) return -1;
  if (xfer->refc==INT_MAX) return -1;
  xfer->refc++;
  return 0;
}

/* New.
 */
 
struct http_xfer *http_xfer_new(int role) {
  struct http_xfer *xfer=calloc(1,sizeof(struct http_xfer));
  if (!xfer) return 0;
  xfer->refc=1;
  xfer->role=role;
  xfer->stage=HTTP_XFER_STAGE_IDLE;
  return xfer;
}

/* Dissect role.
 */
 
int http_xfer_is_request(const struct http_xfer *xfer) {
  switch (xfer->role) {
    case HTTP_XFER_ROLE_SERVER_REQUEST:
    case HTTP_XFER_ROLE_CLIENT_REQUEST:
      return 1;
  }
  return 0;
}

int http_xfer_is_response(const struct http_xfer *xfer) {
  switch (xfer->role) {
    case HTTP_XFER_ROLE_CLIENT_RESPONSE:
    case HTTP_XFER_ROLE_SERVER_RESPONSE:
      return 1;
  }
  return 0;
}

int http_xfer_is_client(const struct http_xfer *xfer) {
  switch (xfer->role) {
    case HTTP_XFER_ROLE_CLIENT_REQUEST:
    case HTTP_XFER_ROLE_CLIENT_RESPONSE:
      return 1;
  }
  return 0;
}

int http_xfer_is_server(const struct http_xfer *xfer) {
  switch (xfer->role) {
    case HTTP_XFER_ROLE_SERVER_REQUEST:
    case HTTP_XFER_ROLE_SERVER_RESPONSE:
      return 1;
  }
  return 0;
}

/* Split line1.
 */
 
int http_xfer_get_method(void *dstpp,const struct http_xfer *xfer) {
  if (!http_xfer_is_request(xfer)) return -1;
  return http_method_from_request_line(dstpp,xfer->line1,xfer->line1c);
}
 
int http_xfer_get_path(void *dstpp,const struct http_xfer *xfer) {
  if (!http_xfer_is_request(xfer)) return -1;
  return http_path_from_request_line(dstpp,xfer->line1,xfer->line1c);
}
 
int http_xfer_get_full_path(void *dstpp,const struct http_xfer *xfer) {
  if (!http_xfer_is_request(xfer)) return -1;
  return http_full_path_from_request_line(dstpp,xfer->line1,xfer->line1c);
}

/* Set line1.
 */
 
int http_xfer_set_request_line(struct http_xfer *xfer,const char *src,int srcc) {
  if (!xfer) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc<=xfer->line1c) {
    memcpy(xfer->line1,src,srcc);
    xfer->line1[srcc]=0;
    xfer->line1c=srcc;
  } else {
    char *nv=malloc(srcc+1);
    if (!nv) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
    if (xfer->line1) free(xfer->line1);
    xfer->line1=nv;
    xfer->line1c=srcc;
  }
  return 0;
}

int http_xfer_set_status_line(struct http_xfer *xfer,const char *src,int srcc) {
  return http_xfer_set_request_line(xfer,src,srcc); // for now at least, they're the same thing.
}

/* Get header.
 */
 
int http_xfer_get_header(void *dstpp,const struct http_xfer *xfer,const char *k,int kc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  const struct http_header *header=xfer->headerv;
  int i=xfer->headerc;
  for (;i-->0;header++) {
    if (kc!=header->kc) continue;
    if (fmn_memcasecmp(k,header->k,kc)) continue;
    *(void**)dstpp=header->v;
    return header->vc;
  }
  return -1;
}

/* Add header.
 */
 
int http_xfer_add_header(struct http_xfer *xfer,const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  if (xfer->headerc>=xfer->headera) {
    int na=xfer->headera+16;
    if (na>INT_MAX/sizeof(struct http_header)) return -1;
    void *nv=realloc(xfer->headerv,sizeof(struct http_header)*na);
    if (!nv) return -1;
    xfer->headerv=nv;
    xfer->headera=na;
  }
  char *nk=malloc(kc+1);
  if (!nk) return -1;
  char *nv=malloc(vc+1);
  if (!nv) { free(nk); return -1; }
  memcpy(nk,k,kc); nk[kc]=0;
  memcpy(nv,v,vc); nv[vc]=0;
  struct http_header *header=xfer->headerv+xfer->headerc++;
  header->k=nk;
  header->v=nv;
  header->kc=kc;
  header->vc=vc;
  return 0;
}
 
int http_xfer_add_header_line(struct http_xfer *xfer,const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *k=src+srcp;
  int kc=0;
  while ((srcp<srcc)&&(src[srcp++]!=':')) kc++;
  while (kc&&((unsigned char)k[kc-1]<=0x20)) kc--;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *v=src+srcp;
  int vc=srcc-srcp;
  while (vc&&((unsigned char)v[vc-1]<=0x20)) vc--;
  return http_xfer_add_header(xfer,k,kc,v,vc);
}

/* Set body.
 */
 
int http_xfer_set_body(struct http_xfer *xfer,const void *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (((char*)src)[srcc]) srcc++; }
  if (srcc>xfer->bodya) {
    void *nv=realloc(xfer->body,srcc);
    if (!nv) return -1;
    xfer->body=nv;
    xfer->bodya=srcc;
  }
  memcpy(xfer->body,src,srcc);
  xfer->bodyc=srcc;
  return 0;
}

int http_xfer_get_body(void *dstpp,const struct http_xfer *xfer) {
  *(void**)dstpp=xfer->body;
  return xfer->bodyc;
}

/* Stage changes.
 */
 
static void http_xfer_begin_IDLE(struct http_xfer *xfer) {
  xfer->stage=HTTP_XFER_STAGE_IDLE;
  xfer->expect=-1;
  xfer->bodyc=0;
}
 
static void http_xfer_begin_LINE1(struct http_xfer *xfer) {
  xfer->stage=HTTP_XFER_STAGE_LINE1;
  xfer->expect=-1;
}

static void http_xfer_begin_HEADER(struct http_xfer *xfer) {
  xfer->stage=HTTP_XFER_STAGE_HEADER;
  xfer->expect=-1;
  while (xfer->headerc>0) {
    xfer->headerc--;
    http_header_cleanup(xfer->headerv+xfer->headerc);
  }
}

static void http_xfer_begin_BODY(struct http_xfer *xfer,int len) {
  xfer->stage=HTTP_XFER_STAGE_BODY;
  xfer->expect=len;
}

static void http_xfer_begin_CHUNKLEN(struct http_xfer *xfer) {
  xfer->stage=HTTP_XFER_STAGE_CHUNKLEN;
  xfer->expect=-1;
}

static void http_xfer_begin_CHUNK(struct http_xfer *xfer,int len) {
  xfer->stage=HTTP_XFER_STAGE_CHUNK;
  xfer->expect=len;
}

/* End of body.
 */

static int http_xfer_finish_body(struct http_xfer *xfer) {
  switch (xfer->role) {
  
    case HTTP_XFER_ROLE_SERVER_REQUEST: {
        if (!xfer->conn||!xfer->context) {
          fprintf(stderr,
            "%s:%d: Received HTTP request but conn or context unset (%p,%p) xfer=%p\n",
            __FILE__,__LINE__,xfer->conn,xfer->context,xfer
          );
          return -1;
        }
        struct http_listener *listener=http_context_find_listener_for_request(xfer->context,xfer);
        struct http_xfer *rsp=http_context_generate_xfer(xfer->context);
        if (!rsp) return -1;
        rsp->role=HTTP_XFER_ROLE_SERVER_RESPONSE;
        int err;
        if (listener) err=listener->serve(listener,xfer,rsp);
        else err=http_xfer_respond(rsp,404,"Listener not found");
        if ((err<0)&&(http_xfer_respond(rsp,500,"Error generating response")<0)) {
          http_xfer_del(rsp);
          return -1;
        }
        http_xfer_encode(xfer->conn,rsp);
        http_xfer_del(rsp);
      } break;
      
    case HTTP_XFER_ROLE_CLIENT_RESPONSE: {
        fprintf(stderr,"TODO:%s: find the client to deliver this %d-byte body to\n",__func__,xfer->bodyc);
      } break;
  }
  http_xfer_begin_IDLE(xfer);
  return 0;
}

/* End of headers.
 */

static int http_xfer_finish_header(struct http_xfer *xfer) {
  
  const char *trenc=0;
  int trencc=http_xfer_get_header(&trenc,xfer,"Transfer-Encoding",17);
  if ((trencc==7)&&!fmn_memcasecmp(trenc,"chunked",7)) {
    http_xfer_begin_CHUNKLEN(xfer);
    return 0;
  }
  
  const char *conlen=0;
  int conlenc=http_xfer_get_header(&conlen,xfer,"Content-Length",14);
  if (conlenc>0) {
    int v;
    if (fmn_int_eval(&v,conlen,conlenc)>=2) { // TODO signal an error instead of skipping past
      http_xfer_begin_BODY(xfer,v);
    }
    return 0;
  }
  
  // No body present, so the transfer is complete. (NB: We're not doing trailers)
  return http_xfer_finish_body(xfer);
}

/* Deliver body or partial body.
 */
 
static int http_xfer_require_body(struct http_xfer *xfer,int addc) {
  if (addc<1) return 0;
  if (xfer->bodyc<=xfer->bodya-addc) return 0;
  if (xfer->bodyc>INT_MAX-addc) return -1;
  int na=xfer->bodyc+addc;
  if (na<INT_MAX-1024) na=(na+1024)&~1023;
  void *nv=realloc(xfer->body,na);
  if (!nv) return -1;
  xfer->body=nv;
  xfer->bodya=na;
  return 0;
}
 
static int http_xfer_deliver_body(struct http_xfer *xfer,const void *src,int srcc) {
  if (http_xfer_require_body(xfer,srcc)<0) return -1;
  memcpy((char*)xfer->body+xfer->bodyc,src,srcc);
  xfer->bodyc+=srcc;
  return 0;
}

/* Receive a cut block of input.
 */
 
static int http_xfer_receive_block(struct http_xfer *xfer,const uint8_t *src,int srcc) {
  switch (xfer->stage) {
  
    case HTTP_XFER_STAGE_LINE1: {
        if (http_xfer_is_request(xfer)) {
          if (http_xfer_set_request_line(xfer,(char*)src,srcc)<0) return -1;
        } else {
          if (http_xfer_set_status_line(xfer,(char*)src,srcc)<0) return -1;
        }
        http_xfer_begin_HEADER(xfer);
      } return 0;
      
    case HTTP_XFER_STAGE_HEADER: {
        while (srcc&&(src[srcc-1]<=0x20)) srcc--;
        while (srcc&&(src[0]<=0x20)) { src++; srcc--; }
        if (srcc) {
          if (http_xfer_add_header_line(xfer,(char*)src,srcc)<0) return -1;
        } else {
          http_xfer_finish_header(xfer);
        }
      } return 0;
      
    case HTTP_XFER_STAGE_BODY: {
        if (http_xfer_deliver_body(xfer,src,srcc)<0) return -1;
        http_xfer_finish_body(xfer);
      } return 0;
      
    case HTTP_XFER_STAGE_CHUNKLEN: {
        while (srcc&&(src[srcc-1]<=0x20)) srcc--;
        while (srcc&&(src[0]<=0x20)) { src++; srcc--; }
        int len=0;
        for (;srcc-->0;src++) {
          int digit=fmn_digit_eval((char)*src);
          if ((digit<0)||(digit>=0x10)) return -1;
          if (len>INT_MAX>>4) return -1;
          len<<=4;
          len|=digit;
        }
        if (len) {
          http_xfer_begin_CHUNK(xfer,len);
        } else {
          http_xfer_finish_body(xfer);
        }
      } return 0;
      
    case HTTP_XFER_STAGE_CHUNK: {
        if (http_xfer_deliver_body(xfer,src,srcc)<0) return -1;
        http_xfer_begin_CHUNKLEN(xfer);
      } return 0;
      
  }
  return -1;
}

/* Receive serial data.
 */

int http_xfer_receive(struct http_xfer *xfer,const void *src,int srcc) {
  if (!xfer||(srcc<0)||(srcc&&!src)) return -1;
  const uint8_t *SRC=src;
  
  // In IDLE stage, eat any whitespace, then advance to LINE1.
  if (xfer->stage==HTTP_XFER_STAGE_IDLE) {
    int srcp=0;
    while ((srcp<srcc)&&(SRC[srcp]<=0x20)) srcp++;
    if (srcp) return srcp;
    http_xfer_begin_LINE1(xfer);
  }
  
  // If we have an explicit expectation, wait for it all to get queued.
  if (xfer->expect>=0) {
    if (srcc<xfer->expect) return 0;
    srcc=xfer->expect;
    xfer->expect=-1;
    if (http_xfer_receive_block(xfer,src,srcc)<0) return -1;
    return srcc;
  }
  
  // Otherwise, consume through LF or nothing.
  int srcp=0;
  while (srcp<srcc) {
    if (SRC[srcp++]==0x0a) {
      if (http_xfer_receive_block(xfer,src,srcp)<0) return -1;
      return srcp;
    }
  }
  return 0;
}

/* Set up bodyless response.
 */

int http_xfer_respond(struct http_xfer *xfer,int status,const char *fmt,...) {
  if (!fmt) fmt="";
  va_list vargs;
  va_start(vargs,fmt);
  char tmp[128];
  int tmpc=vsnprintf(tmp,sizeof(tmp),fmt,vargs);
  if ((tmpc<0)||(tmpc>=sizeof(tmp))) tmpc=0;
  char line[192];
  int linec=snprintf(line,sizeof(line),"HTTP/1.1 %03d %.*s",status,tmpc,tmp);
  if ((linec<1)||(linec>=sizeof(line))) return -1;
  return http_xfer_set_status_line(xfer,line,linec);
}

/* Encode xfer to conn's output.
 */
 
int http_xfer_encode(struct http_conn *conn,struct http_xfer *xfer) {
  if (!conn||!xfer) return -1;
  if (http_conn_wbuf_append(conn,xfer->line1,xfer->line1c)<0) return -1;
  if (http_conn_wbuf_append(conn,"\r\n",2)<0) return -1;
  const struct http_header *header=xfer->headerv;
  int i=xfer->headerc;
  for (;i-->0;header++) {
    if (http_conn_wbuf_appendf(conn,"%.*s: %.*s\r\n",header->kc,header->k,header->vc,header->v)<0) return -1;
  }
  if (http_xfer_is_server(xfer)||xfer->bodyc) {
    if (http_conn_wbuf_appendf(conn,"Content-Length: %d\r\n\r\n",xfer->bodyc)<0) return -1;
    if (http_conn_wbuf_append(conn,xfer->body,xfer->bodyc)<0) return -1;
  } else {
    if (http_conn_wbuf_append(conn,"\r\n",2)<0) return -1;
  }
  return 0;
}
