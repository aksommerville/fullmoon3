#include "fmn_editor_internal.h"

/* Resolve empty, dot, and double-dot components.
 * Turn backslashes into slashes.
 * Fail on double-dot if it exceeds the start of the path.
 * The output can't be longer than the input.
 */
 
static int fmn_editor_normalize_path_in_place(char *path,int pathc) {
  int srcp=0,dstp=0;
  while (srcp<pathc) {
  
    if ((path[srcp]=='/')||(path[srcp]=='\\')) { srcp++; continue; }
    const char *token=path+srcp;
    int tokenc=0;
    while ((srcp<pathc)&&(path[srcp]!='/')&&(path[srcp]!='\\')) { srcp++; tokenc++; }
    if (!tokenc) continue; // not possible
    if ((tokenc==1)&&(token[0]=='.')) continue; // equivalent to empty
    
    if ((tokenc==2)&&(token[0]=='.')&&(token[1]=='.')) {
      if (!dstp) return -1;
      if ((dstp==1)&&((path[0]=='/')||(path[0]=='\\'))) return -1;
      if ((path[dstp-1]=='/')||(path[dstp-1]=='\\')) dstp--;
      while (dstp&&(path[dstp-1]!='/')&&(path[dstp-1]!='\\')) dstp--;
      continue;
    }
    
    if (dstp&&(path[dstp-1]!='/')&&(path[dstp-1]!='\\')) {
      path[dstp++]='/'; //TODO backslash here, if we build for Windows
    }
    
    memmove(path+dstp,token,tokenc);
    dstp+=tokenc;
  }
  path[dstp]=0;
  return dstp;
}

/* Produce local path from request.
 * We apply the local htdocs root, resolve dots, and ensure it's still under the root.
 */
 
static int fmn_editor_resolve_http_path(char *dst,int dsta,const char *src,int srcc) {
  if (!dst||(dsta<0)) dsta=0;
  
  const char *htdocs="src/tool/editor/www";//TODO configurable
  if ((srcc>=5)&&!memcmp(src,"/src/",5)) {
    htdocs="src";
    src+=5;
    srcc-=5;
  }
  
  int htdocsc=0; while (htdocs[htdocsc]) htdocsc++;
  int dstc=snprintf(dst,dsta,"%.*s/%.*s",htdocsc,htdocs,srcc,src);
  if ((dstc<1)||(dstc>=dsta)) return -1;
  dstc=fmn_editor_normalize_path_in_place(dst,dstc);
  if (dstc<htdocsc) return -1;
  if (memcmp(dst,htdocs,htdocsc)) return -1;
  if ((dstc>htdocsc)&&(dst[htdocsc]!='/')&&(dst[htdocsc]!='\\')) return -1;
  return dstc;
}

/* GET
 */
 
static int fmn_editor_GET_static(struct http_xfer *rsp,const char *path) {

  // First try to read it as a regular file. If that works, return it, easy.
  void *src=0;
  int srcc=fmn_file_read(&src,path);
  if (srcc>=0) {
    if (http_xfer_set_body(rsp,src,srcc)<0) {
      free(src);
      return -1;
    }
    const char *contype=http_guess_content_type(path,src,srcc);
    free(src);
    http_xfer_add_header(rsp,"Content-Type",12,contype,-1);
    return http_xfer_respond(rsp,200,"OK");
  }
  
  // Append "/index.html" and try again.
  // We don't do generic directory listings, just this one special case.
  char indexpath[1024];
  int indexpathc=snprintf(indexpath,sizeof(indexpath),"%s/index.html",path);
  if ((indexpathc<1)||(indexpathc>=sizeof(indexpath))) return http_xfer_respond(rsp,404,"Not found");
  if ((srcc=fmn_file_read(&src,indexpath))>=0) {
    if (http_xfer_set_body(rsp,src,srcc)<0) {
      free(src);
      return -1;
    }
    free(src);
    http_xfer_add_header(rsp,"Content-Type",12,"text/html",9);
    return http_xfer_respond(rsp,200,"OK");
  }
  
  return http_xfer_respond(rsp,404,"Not found");
}

/* PUT
 */
 
static int fmn_editor_PUT_static(struct http_xfer *rsp,const char *path,struct http_xfer *req) {
  const void *body=0;
  int bodyc=http_xfer_get_body(&body,req);
  if (bodyc<0) bodyc=0;
  if (fmn_file_write(path,body,bodyc)<0) {
    return http_xfer_respond(rsp,500,"Failed to write file");
  } else {
    return http_xfer_respond(rsp,201,"Created");
  }
}

/* DELETE
 */
 
static int fmn_editor_DELETE_static(struct http_xfer *rsp,const char *path) {
  if (fmn_file_rm(path)<0) {
    return http_xfer_respond(rsp,404,"Not found");
  } else {
    return http_xfer_respond(rsp,204,"Deleted");
  }
}

/* Serve static files, main entry point.
 */
 
int fmn_editor_serve_static(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp) {

  const char *rpath,*method;
  int methodc=http_xfer_get_method(&method,req);
  int rpathc=http_xfer_get_path(&rpath,req);
  if (methodc<0) methodc=0;
  if (rpathc<0) rpathc=0;
  
  char path[1024];
  int pathc=fmn_editor_resolve_http_path(path,sizeof(path),rpath,rpathc);
  if ((pathc<1)||(pathc>=sizeof(path))) return http_xfer_respond(rsp,400,"Invalid path");
  
  if ((methodc==3)&&!memcmp(method,"GET",3)) return fmn_editor_GET_static(rsp,path);
  if ((methodc==6)&&!memcmp(method,"DELETE",6)) return fmn_editor_DELETE_static(rsp,path);
  
  if ((methodc==3)&&!memcmp(method,"PUT",3)) {
    return fmn_editor_PUT_static(rsp,path,req);
  }
  
  return http_xfer_respond(rsp,405,"Method '%.*s' not allowed",methodc,method);
}
