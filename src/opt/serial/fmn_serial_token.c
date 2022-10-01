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
