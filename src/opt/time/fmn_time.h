/* fmn_time.h
 */
 
#ifndef FMN_TIME_H
#define FMN_TIME_H

#include <stdint.h>

int64_t fmn_now_us();
double fmn_now_s();
int64_t fmn_now_cpu_us();
double fmn_now_cpu_s();

void fmn_sleep_us(int us);
void fmn_sleep_s(double s);

#endif
