#include "fmn_resmgr.h"
#include "opt/fs/fmn_fs.h"
#include <string.h>
#include <stdio.h>

/* Normalize path suffix.
 */
 
static int fmn_resmgr_normalize_suffix(char *dst,int dsta,const char *src,int srcc) {
  if (!dst||(dsta<0)) dsta=0;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int sfxp=srcc;
  while (sfxp&&(src[sfxp-1]!='.')&&(src[sfxp-1]!='/')) sfxp--;
  int dstc=srcc-sfxp;
  if (dstc>dsta) return dstc;
  int i=dstc; while (i-->0) {
    char ch=src[sfxp+i];
    if ((ch>='A')&&(ch<='Z')) dst[i]=ch+0x20;
    else dst[i]=ch;
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Restype strings.
 */
 
int fmn_restype_eval(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return FMN_RESTYPE_##tag;
  FMN_FOR_EACH_RESTYPE
  #undef _
  return -1;
}

const char *fmn_restype_repr(int restype) {
  switch (restype) {
    #define _(tag) case FMN_RESTYPE_##tag: return #tag;
    FMN_FOR_EACH_RESTYPE
    #undef _
  }
  return 0;
}

/* List files.
 */
 
struct fmn_resmgr_list_files_context {
  int (*cb)(const char *path,int type,const char *name,int namec,void *userdata);
  void *userdata;
  int restype;
};

static int fmn_resmgr_list_files_cb(const char *path,const char *base,char type,void *userdata) {
  struct fmn_resmgr_list_files_context *ctx=userdata;
  if (!type) type=fmn_file_get_type(path);
  
  // Directories with the exact name of a restype push that type. Then recur.
  if (type=='d') {
    int pvrestype=ctx->restype;
    if ((ctx->restype=fmn_restype_eval(base,-1))<0) ctx->restype=pvrestype;
    int err=fmn_dir_read(path,fmn_resmgr_list_files_cb,userdata);
    ctx->restype=pvrestype;
    return err;
  }
  
  // Regular files generally are resources, if we can determine restype.
  if (type=='f') {
    char sfx[16];
    int sfxc=fmn_resmgr_normalize_suffix(sfx,sizeof(sfx),base,-1);
    const char *name=base;
    int namec=0;
    for (;name[namec];namec++) {
      if ((name[namec]>='a')&&(name[namec]<='z')) continue;
      if ((name[namec]>='A')&&(name[namec]<='Z')) continue;
      if ((name[namec]>='0')&&(name[namec]<='9')) continue;
      if (name[namec]=='_') continue;
      break;
    }
    if (!namec) return 0;
    
    // Appendix files, we take the suffix on faith.
    if ((sfxc==6)&&!memcmp(sfx,"adjust",6)) return ctx->cb(path,FMN_RESTYPE_adjust,name,namec,ctx->userdata);
    if ((sfxc==9)&&!memcmp(sfx,"tileprops",9)) return ctx->cb(path,FMN_RESTYPE_tileprops,name,namec,ctx->userdata);
    
    // Use restype if we have it from context (ie from a parent directory).
    if (ctx->restype) return ctx->cb(path,ctx->restype,name,namec,ctx->userdata);
    
    // Pick off known singleton files.
    if (!strcmp(base,"waves.txt")) return ctx->cb(path,FMN_RESTYPE_waves,"waves",5,ctx->userdata);
    
    // I was thinking to guess restype from (sfx) but that doesn't make sense; our Makefile doesn't do that.
    return 0;
  }
  
  return 0;
}

int fmn_resmgr_list_files(
  int (*cb)(const char *path,int type,const char *name,int namec,void *userdata),
  void *userdata
) {
  struct fmn_resmgr_list_files_context ctx={
    .cb=cb,
    .userdata=userdata,
    .restype=0,
  };
  return fmn_dir_read("src/data",fmn_resmgr_list_files_cb,&ctx);
}
