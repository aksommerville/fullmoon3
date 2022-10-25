#include "fmn_serial.h"
#include <limits.h>

/* Evaluate integer.
 */
 
int fmn_int_eval(int *dst,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0,base=10,positive=1;
  
  if (srcp>=srcc) return -1;
  if (src[srcp]=='-') {
    if (++srcp>=srcc) return -1;
    positive=0;
  } else if (src[srcp]=='+') {
    if (++srcp>=srcc) return -1;
  }
  
  if ((srcp<srcc-2)&&(src[srcp]=='0')) switch (src[srcp+1]) {
    case 'x': case 'X': base=16; srcp+=2; break;
    case 'd': case 'D': base=10; srcp+=2; break;
    case 'o': case 'O': base= 8; srcp+=2; break;
    case 'b': case 'B': base= 2; srcp+=2; break;
  }
  
  int limit,overflow=0;
  if (positive) limit=UINT_MAX/base;
  else limit=INT_MIN/base;
  *dst=0;
  
  while (srcp<srcc) {
    int digit=fmn_digit_eval(src[srcp++]);
    if ((digit<0)||(digit>=base)) return -1;
    if (positive) {
      if ((unsigned int)(*dst)>(unsigned int)limit) overflow=1;
      (*dst)*=base;
      if ((unsigned int)(*dst)>UINT_MAX-digit) overflow=1;
      (*dst)+=digit;
    } else {
      if (*dst<limit) overflow=1;
      (*dst)*=base;
      if (*dst<INT_MIN+digit) overflow=1;
      (*dst)-=digit;
    }
  }
  
  if (overflow) return 0;
  if (positive&&(*dst<0)) return 1;
  return 2;
}

/* Represent integer.
 */
 
int fmn_decsint_repr(char *dst,int dsta,int src) {
  if (!dst||(dsta<0)) dsta=0;
  if (src<0) {
    int dstc=2,limit=-10;
    while (src<=limit) { dstc++; if (limit<INT_MIN/10) break; limit*=10; }
    if (dstc>dsta) return dstc;
    int i=dstc; for (;i-->0;src/=10) dst[i]='0'-src%10;
    dst[0]='-';
    if (dstc<dsta) dst[dstc]=0;
    return dstc;
  } else {
    int dstc=1,limit=10;
    while (src>=limit) { dstc++; if (limit>INT_MAX/10) break; limit*=10; }
    if (dstc>dsta) return dstc;
    int i=dstc; for (;i-->0;src/=10) dst[i]='0'+src%10;
    if (dstc<dsta) dst[dstc]=0;
    return dstc;
  }
}

/* Split comma-delimited string with callbacks.
 */
 
int fmn_for_each_comma_string(const char *src,int srcc,int (*cb)(const char *src,int srcc,void *userdata),void *userdata) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0,err;
  while (srcp<srcc) {
    const char *sub=src+srcp;
    int subc=0;
    while ((srcp<srcc)&&(src[srcp++]!=',')) subc++;
    if (err=cb(sub,subc,userdata)) return err;
  }
  return 0;
}

/* Case-insensitive memcmp.
 */
 
int fmn_memcasecmp(const void *a,const void *b,int c) {
  if (a==b) return 0;
  if (c<1) return 0;
  while (c-->0) {
    uint8_t cha=*(uint8_t*)a,chb=*(uint8_t*)b;
    if ((cha>=0x41)&&(cha<=0x5a)) cha+=0x20;
    if ((chb>=0x41)&&(chb<=0x5a)) chb+=0x20;
    if (cha<chb) return -1;
    if (cha>chb) return 1;
    a=(uint8_t*)a+1;
    b=(uint8_t*)b+1;
  }
  return 0;
}

/* Match strings verbatim, except '*' wildcard.
 */
 
int fmn_wildcard_match(const char *pat,int patc,const char *src,int srcc) {
  if (!pat) patc=0; else if (patc<0) { patc=0; while (pat[patc]) patc++; }
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int patp=0,srcp=0;
  while (1) {
  
    // If they're both terminated, success.
    // If (pat) terminated, failure.
    // Don't check (src) termination just yet; a star can still match.
    if (patp>=patc) {
      if (srcp>=srcc) return 1;
      return 0;
    }
    
    // Check stars.
    if (pat[patp]=='*') {
      while ((patp<patc)&&(pat[patp]=='*')) patp++; // adjacent stars are redundant
      if (patp>=patc) return 1; // a trailing star matches everything
      while (srcp<srcc) {
        if (fmn_wildcard_match(pat+patp,patc-patp,src+srcp,srcc-srcp)) return 1;
        srcp++;
      }
      return 0;
    }
    
    // (src) terminated, failure.
    if (srcp>=srcc) return 0;
    
    // Everything else must match verbatim.
    char cha=pat[patp++];
    char chb=src[srcp++];
    if (cha!=chb) return 0;
  }
}

/* Evaluate string.
 */
 
int fmn_string_eval(char *dst,int dsta,const char *src,int srcc) {
  if (!dst||(dsta<0)) dsta=0;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if ((srcc<2)||(src[0]!=src[srcc-1])) return -1;
  if ((src[0]!='"')&&(src[0]!='\'')&&(src[0]!='`')) return -1;
  src++; srcc-=2;
  int dstc=0,srcp=0;
  while (srcp<srcc) {
    if (src[srcp]=='\\') {
      if (++srcp>=srcc) return -1;
      switch (src[srcp++]) {
        case '\\': case '"': case '\'': case '`': case '/': if (dstc<dsta) dst[dstc]=src[srcp-1]; dstc++; break;
        case '0': if (dstc<dsta) dst[dstc]=0x00; dstc++; break;
        case 'b': if (dstc<dsta) dst[dstc]=0x08; dstc++; break;
        case 't': if (dstc<dsta) dst[dstc]=0x09; dstc++; break;
        case 'n': if (dstc<dsta) dst[dstc]=0x0a; dstc++; break;
        case 'r': if (dstc<dsta) dst[dstc]=0x0d; dstc++; break;
        case 'x': {
            if (srcp>srcc-2) return -1;
            int hi=fmn_digit_eval(src[srcp++]);
            int lo=fmn_digit_eval(src[srcp++]);
            if ((hi<0)||(hi>15)||(lo<0)||(lo>15)) return -1;
            if (dstc<dsta) dst[dstc]=(hi<<4)|lo;
            dstc++;
          } break;
        case 'u': {
            if (srcp>srcc-4) return -1;
            int ch=0,i=4; while (i-->0) {
              int digit=fmn_digit_eval(src[srcp++]);
              if ((digit<0)||(digit>15)) return -1;
              ch<<=4;
              ch|=digit;
            }
            if (ch>0xff) ch=0xff; // We don't have a UTF-8 encoder, and I'd like to not have one.
            if (dstc<dsta) dst[dstc]=ch;
            dstc++;
            // No special accomodation for surrogate pairs; they'll come out as (0xff,0xff).
          } break;
        default: return -1;
      }
    } else {
      if (dstc<dsta) dst[dstc]=src[srcp];
      dstc++;
      srcp++;
    }
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Represent string.
 */
 
int fmn_string_repr(char *dst,int dsta,const char *src,int srcc) {
  if (!dst||(dsta<0)) dsta=0;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int dstc=0,srcp=0;
  if (dstc<dsta) dst[dstc]='"'; dstc++;
  for (;srcp<srcc;srcp++) { // no multibyte chars
    switch (src[srcp]) {
      case 0x00: if (dstc<=dsta-2) { dst[dstc++]='\\'; dst[dstc++]='0'; } else dstc+=2; break;
      case 0x08: if (dstc<=dsta-2) { dst[dstc++]='\\'; dst[dstc++]='b'; } else dstc+=2; break;
      case 0x09: if (dstc<=dsta-2) { dst[dstc++]='\\'; dst[dstc++]='t'; } else dstc+=2; break;
      case 0x0a: if (dstc<=dsta-2) { dst[dstc++]='\\'; dst[dstc++]='n'; } else dstc+=2; break;
      case 0x0d: if (dstc<=dsta-2) { dst[dstc++]='\\'; dst[dstc++]='r'; } else dstc+=2; break;
      case '\\': if (dstc<=dsta-2) { dst[dstc++]='\\'; dst[dstc++]='\\'; } else dstc+=2; break;
      case  '"': if (dstc<=dsta-2) { dst[dstc++]='\\'; dst[dstc++]='"'; } else dstc+=2; break;
      default: if ((src[srcp]>=0x20)&&(src[srcp]<=0x7e)) {
          if (dstc<dsta) dst[dstc]=src[srcp];
          dstc++;
        } else {
          if (dstc<=dsta-6) {
            dst[dstc++]='\\';
            dst[dstc++]='u';
            dst[dstc++]='0';
            dst[dstc++]='0';
            dst[dstc++]="0123456789abcdef"[(src[srcp]>>4)&15];
            dst[dstc++]="0123456789abcdef"[src[srcp]&15];
          } else dstc+=6;
        }
    }
  }
  if (dstc<dsta) dst[dstc]='"'; dstc++;
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}
