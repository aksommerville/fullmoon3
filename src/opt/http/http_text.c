#include "http.h"
#include <string.h>
#include <stdint.h>

/* Space-delimited token by index.
 */
 
static int http_token_by_index(void *dstpp,const char *src,int srcc,int p) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  while (p-->0) {
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) srcp++;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  }
  const char *token=src+srcp;
  int tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  *(const void**)dstpp=token;
  return tokenc;
}

/* Request-Line.
 */
 
int http_method_from_request_line(void *dstpp,const char *src,int srcc) {
  return http_token_by_index(dstpp,src,srcc,0);
}

int http_full_path_from_request_line(void *dstpp,const char *src,int srcc) {
  return http_token_by_index(dstpp,src,srcc,1);
}

int http_path_from_request_line(void *dstpp,const char *src,int srcc) {
  const char *path;
  int pathc=http_full_path_from_request_line(&path,src,srcc);
  int pfxc=0;
  while ((pfxc<pathc)&&(path[pfxc]!='?')) pfxc++;
  *(const void**)dstpp=path;
  return pfxc;
}

int http_query_from_request_line(void *dstpp,const char *src,int srcc) {
  const char *path;
  int pathc=http_full_path_from_request_line(&path,src,srcc);
  int pathp=0;
  while ((pathp<pathc)&&(path[pathp]!='?')) pathp++;
  if (pathp>=pathc) return 0;
  pathp++; // don't return the introducer
  *(const void**)dstpp=path+pathp;
  return srcc-pathp;
}

int http_status_from_status_line(void *dstpp,const char *src,int srcc) {
  return http_token_by_index(dstpp,src,srcc,1);
}

int http_message_from_status_line(void *dstpp,const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) srcp++; // protocol
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) srcp++; // status
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  *(const void**)dstpp=src+srcp;
  int tokenc=srcc-srcp;
  while (tokenc&&((unsigned char)src[srcp-tokenc]<=0x20)) tokenc--;
  return tokenc;
}

/* Guess Content-Type.
 */

const char *http_guess_content_type(const char *path,const void *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (((char*)src)[srcc]) srcc++; }
  const uint8_t *SRC=src;
  
  // First check unambiguous signatures.
  // These are rarer than I'd like.
  if (src) {
    if ((srcc>=8)&&!memcmp(src,"\x89PNG\r\n\x1a\n",8)) return "image/png";
    if ((srcc>=8)&&!memcmp(src,"MThd\0\0\0\6",8)) return "audio/midi";
  }
  
  // Next consider the path suffix.
  if (path) {
    const char *presfx=0;
    int presfxc=0,pathp=0;
    for (;path[pathp];pathp++) {
      if (path[pathp]=='/') {
        presfx=0;
        presfxc=0;
      } else if (path[pathp]=='.') {
        presfx=path+pathp+1;
        presfxc=0;
      } else if (presfx) {
        presfxc++;
      }
    }
    char sfx[8];
    if ((presfxc>0)&&(presfxc<=sizeof(sfx))) {
      int i=presfxc; while (i-->0) {
        if ((presfx[i]>='A')&&(presfx[i]<='Z')) sfx[i]=presfx[i]+0x20;
        else sfx[i]=presfx[i];
      }
      switch (presfxc) {
        case 1: switch (sfx[0]) {
            case 'c': return "text/plain";
            case 'h': return "text/plain";
            case 'm': return "text/plain";
            case 'S': return "text/plain";
          } break;
        case 2: {
            if (!memcmp(sfx,"js",2)) return "application/javascript";
            if (!memcmp(sfx,"py",2)) return "text/palin";
          } break;
        case 3: {
            if (!memcmp(sfx,"png",3)) return "image/png";
            if (!memcmp(sfx,"gif",3)) return "image/gif";
            if (!memcmp(sfx,"jpg",3)) return "image/jpeg";
            if (!memcmp(sfx,"mid",3)) return "audio/midi";
            if (!memcmp(sfx,"wav",3)) return "audio/wav"; //TODO WAV MIME type?
            if (!memcmp(sfx,"css",3)) return "text/css";
            if (!memcmp(sfx,"xml",3)) return "application/xml";
            if (!memcmp(sfx,"ico",3)) return "image/icon";
          } break;
        case 4: {
            if (!memcmp(sfx,"html",4)) return "text/html";
            if (!memcmp(sfx,"jpeg",4)) return "image/jpeg";
            if (!memcmp(sfx,"json",4)) return "application/json";
          } break;
      }
    }
  }
  
  // Getting a bit unlikely now. Check ambiguous or texty signatures.
  if (src) {
    if ((srcc>=6)&&!memcmp(src,"GIF87a",6)) return "image/gif";
    if ((srcc>=6)&&!memcmp(src,"GIF89a",6)) return "image/gif";
    if ((srcc>=5)&&!memcmp(src,"<?xml",5)) return "application/xml";
    if ((srcc>=14)&&!memcmp(src,"<!DOCTYPE html",14)) return "text/html";
    if ((srcc>=1)&&(SRC[0]=='{')) return "application/json";
  }
  
  // If content is present and it's all ASCII, call it "text/plain".
  // Don't read the whole thing necessarily.
  if (srcc) {
    int ckc=srcc;
    if (ckc>256) ckc=256;
    while (ckc-->0) {
      if ((SRC[ckc]>=0x20)&&(SRC[ckc]<=0x7e)) continue;
      if (SRC[ckc]==0x09) continue;
      if (SRC[ckc]==0x0a) continue;
      if (SRC[ckc]==0x0d) continue;
      return "application/octet-stream";
    }
    return "text/plain";
  }
  return "application/octet-stream";
}
