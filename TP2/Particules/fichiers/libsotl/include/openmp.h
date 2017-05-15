
#ifndef OMPENMP_H
#define OMPENMP_H

#include "device.h"

void omp_init (sotl_device_t *dev);
void omp_alloc_buffers (sotl_device_t *dev);
void omp_finalize (sotl_device_t *dev);

void omp_one_step_move (sotl_device_t *dev);


#endif
