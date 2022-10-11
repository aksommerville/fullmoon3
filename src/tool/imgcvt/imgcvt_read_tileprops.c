#include "imgcvt_internal.h"

/* Read one line from the first table in tileprops.
 */
 
static int imgcvt_tileprops_parse_line(uint8_t *dst,const char *src,int srcc) {
  int dstc=0,srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; }
    int v;
    if (fmn_int_eval(&v,token,tokenc)<2) return -1;
    if ((v<0)||(v>3)) return -1;
    if (dstc>=16) return -1;
    dst[dstc]=v;
    dstc++;
  }
  if (dstc<16) return -1;
  return 0;
}

/* Read tileprops, main entry point.
 */
 
int imgcvt_read_tileprops(const char *path) {

  // No error if read fails -- caller may provide this path on an "if it exists" basis.
  char *src=0;
  int srcc=fmn_file_read(&src,path);
  if (srcc<0) return 0;
  
  int y=0;
  int srcp=0,lineno=0;
  while (srcp<srcc) {
  
    lineno++;
    const char *line=src+srcp;
    int linec=0;
    while (srcp<srcc) {
      if (src[srcp++]==0x0a) break;
      linec++;
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    if (!linec) continue;
    
    if (imgcvt_tileprops_parse_line(imgcvt.tilepropv+y*16,line,linec)<0) {
      fprintf(stderr,
        "%s:%d: Failed to parse tileprops line. Must be 16 integers in 0..3, separated by whitespace.\n",
        path,lineno
      );
      free(src);
      return -2;
    }
    y++;
    if (y>=16) break;
  }
  free(src);
  if (y<1) return 0;
  
  const uint8_t *p=imgcvt.tilepropv;
  int i=64;
  for (;i-->0;p+=4) {
    uint8_t v=(p[0]<<6)|(p[1]<<4)|(p[2]<<2)|p[3];
    if (fmn_encode_u8(&imgcvt.tilepropsbin,v)<0) return -1;
  }
  
  return 0;
}
