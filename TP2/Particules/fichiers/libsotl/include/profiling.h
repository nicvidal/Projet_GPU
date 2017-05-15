#ifndef PROFILING_H
#define PROFILING_H

#include "sotl.h"

void profiling_init (sotl_device_t *dev);
void profiling_finalize (sotl_device_t *dev);

void profiling_reset_counters (sotl_device_t *dev);

#ifdef PROFILING
cl_event *prof_event_ptr(sotl_device_t *dev, unsigned kernel_num);
#else
#define prof_event_ptr(device, kernel)  NULL
#endif

#define TIME_DIFF(t1, t2) \
    ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

#endif
