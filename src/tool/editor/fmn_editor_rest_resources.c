#include "fmn_editor_internal.h"

/* GET /api/restoc
 */
 
static int fmn_editor_restoc_cb(const char *path,int restype,const char *name,int namec,void *userdata) {
  struct fmn_encoder *dst=userdata;
  int jsonctx=fmn_encode_json_object_start(dst,0,0);
  if (jsonctx<0) return -1;
  if (fmn_encode_json_string(dst,"type",4,fmn_restype_repr(restype),-1)<0) return -1;
  if (fmn_encode_json_string(dst,"name",4,name,namec)<0) return -1;
  if (fmn_encode_json_string(dst,"path",4,path,-1)<0) return -1;
  if (fmn_encode_json_object_end(dst,jsonctx)<0) return -1;
  return 0;
}
 
static int fmn_editor_restoc_run(struct fmn_encoder *dst) {
  int jsonctx=fmn_encode_json_array_start(dst,0,0);
  if (jsonctx<0) return -1;
  if (fmn_resmgr_list_files(fmn_editor_restoc_cb,dst)<0) return -1;
  return fmn_encode_json_array_end(dst,jsonctx);
}
 
int fmn_editor_GET_restoc(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp) {
  struct fmn_encoder dst={0};
  if (fmn_editor_restoc_run(&dst)<0) {
    fmn_encoder_cleanup(&dst);
    return http_xfer_respond(rsp,500,"");
  }
  http_xfer_add_header(rsp,"Content-Type",12,"application/json",16);
  http_xfer_set_body(rsp,dst.v,dst.c);
  fmn_encoder_cleanup(&dst);
  return http_xfer_respond(rsp,200,"OK");
}

/* GET /api/resall
 */
 
static int fmn_editor_resall_cb(const char *path,int restype,const char *name,int namec,void *userdata) {
  struct fmn_encoder *dst=userdata;
  int jsonctx=fmn_encode_json_object_start(dst,0,0);
  if (jsonctx<0) return -1;
  if (fmn_encode_json_string(dst,"type",4,fmn_restype_repr(restype),-1)<0) return -1;
  if (fmn_encode_json_string(dst,"name",4,name,namec)<0) return -1;
  if (fmn_encode_json_string(dst,"path",4,path,-1)<0) return -1;
  
  // Decision of whether to pre-load a resource gets made right here, capriciously.
  // We don't load binary files. If I change my mind on that, we'll need a base64 encoder.
  // (but as it happens: "image" should load separately with browser facilities, and "song" is unlikely to be used by our editor).
  switch (restype) {
    #define NOLOAD(msg) if (fmn_encode_json_string(dst,"desc",4,msg,-1)<0) return -1;
    #define TEXT { \
      void *src=0; \
      int srcc=fmn_file_read(&src,path); \
      if (srcc>=0) { \
        int err=fmn_encode_json_string(dst,"text",4,src,srcc); \
        free(src); \
        if (err<0) return err; \
      } else { \
        if (fmn_encode_json_string(dst,"desc",4,"Failed to read file.",-1)<0) return -1; \
      } \
    }
    case FMN_RESTYPE_image: NOLOAD("Load images individually.") break;
    case FMN_RESTYPE_song: NOLOAD("Load songs individually if you want them.") break;
    case FMN_RESTYPE_waves: TEXT break;
    case FMN_RESTYPE_map: TEXT break;
    case FMN_RESTYPE_tileprops: TEXT break;
    case FMN_RESTYPE_adjust: TEXT break;
    default: NOLOAD("Unknown resource type") break;
    #undef NOLOAD
    #undef TEXT
  }
  
  if (fmn_encode_json_object_end(dst,jsonctx)<0) return -1;
  return 0;
}
 
static int fmn_editor_resall_run(struct fmn_encoder *dst) {
  int jsonctx=fmn_encode_json_array_start(dst,0,0);
  if (jsonctx<0) return -1;
  if (fmn_resmgr_list_files(fmn_editor_resall_cb,dst)<0) return -1;
  return fmn_encode_json_array_end(dst,jsonctx);
}
 
int fmn_editor_GET_resall(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp) {
  struct fmn_encoder dst={0};
  if (fmn_editor_resall_run(&dst)<0) {
    fmn_encoder_cleanup(&dst);
    return http_xfer_respond(rsp,500,"");
  }
  http_xfer_add_header(rsp,"Content-Type",12,"application/json",16);
  http_xfer_set_body(rsp,dst.v,dst.c);
  fmn_encoder_cleanup(&dst);
  return http_xfer_respond(rsp,200,"OK");
}
