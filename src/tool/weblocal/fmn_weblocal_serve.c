#include "fmn_weblocal_internal.h"
#include <sys/stat.h>

/* Resolve empty, dot, and double-dot components.
 * Turn backslashes into slashes.
 * Fail on double-dot if it exceeds the start of the path.
 * The output can't be longer than the input.
 */
 
static int fmn_weblocal_normalize_path_in_place(char *path,int pathc) {
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
 
static int fmn_weblocal_resolve_http_path(char *dst,int dsta,const char *src,int srcc) {
  if (!dst||(dsta<0)) dsta=0;
  int i=0;
  for (;i<fmn_weblocal.htdocsc;i++) {
    const char *htdocs=fmn_weblocal.htdocsv[i];
    int htdocsc=0; while (htdocs[htdocsc]) htdocsc++;
    int dstc=snprintf(dst,dsta,"%.*s/%.*s",htdocsc,htdocs,srcc,src);
    if ((dstc<1)||(dstc>=dsta)) continue;
    dstc=fmn_weblocal_normalize_path_in_place(dst,dstc);
    if (dstc<htdocsc) continue;
    if (memcmp(dst,htdocs,htdocsc)) continue;
    if ((dstc>htdocsc)&&(dst[htdocsc]!='/')&&(dst[htdocsc]!='\\')) continue;
    struct stat st;
    if (stat(dst,&st)<0) continue;
    return dstc;
  }
  return -1;
}

/* GET
 */
 
static int fmn_weblocal_GET_static(struct http_xfer *rsp,const char *path) {

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

/* Serve static files, main entry point.
 */
 
int fmn_weblocal_serve_static(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp) {
  const char *rpath;
  int rpathc=http_xfer_get_path(&rpath,req);
  if (rpathc<0) rpathc=0;
  char path[1024];
  int pathc=fmn_weblocal_resolve_http_path(path,sizeof(path),rpath,rpathc);
  if ((pathc<1)||(pathc>=sizeof(path))) return http_xfer_respond(rsp,400,"Invalid path");
  return fmn_weblocal_GET_static(rsp,path);
}
