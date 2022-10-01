/* fmn_serial.h
 */
 
#ifndef FMN_SERIAL_H
#define FMN_SERIAL_H

/* Primitive tokens.
 *****************************************************/
 
static inline int fmn_digit_eval(char src) {
  if ((src>='0')&&(src<='9')) return src-'0';
  if ((src>='a')&&(src<='z')) return src-'a'+10;
  if ((src>='A')&&(src<='Z')) return src-'A'+10;
  return -1;
}
 
int fmn_int_eval(int *dst,const char *src,int srcc);

int fmn_for_each_comma_string(const char *src,int srcc,int (*cb)(const char *src,int srcc,void *userdata),void *userdata);

#endif
