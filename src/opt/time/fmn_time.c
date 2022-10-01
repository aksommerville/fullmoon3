#include "fmn_time.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

int64_t fmn_now_us() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (int64_t)tv.tv_sec*1000000ll+tv.tv_usec;
}

double fmn_now_s() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
}

int64_t fmn_now_cpu_us() {
  struct timespec tv={0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
  return (int64_t)tv.tv_sec*1000000ll+tv.tv_nsec/1000;
}

double fmn_now_cpu_s() {
  struct timespec tv={0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
  return (double)tv.tv_sec+(double)tv.tv_nsec/1000000000.0;
}

void fmn_sleep_us(int us) {
  usleep(us);
}

void fmn_sleep_s(double s) {
  usleep(s*1000000.0);
}
