#ifndef _OCL_IS_DEF
#define _OCL_IS_DEF

#include <stdio.h>

#include "atom.h"
#include "device.h"
#include "default_defines.h"
#include "cl.h"

// Initialize OpenCL resources associated to device
//
void ocl_init(sotl_device_t *dev);

void ocl_acquire(sotl_device_t *dev);
void ocl_release(sotl_device_t *dev);

void ocl_alloc_buffers(sotl_device_t *dev);
void ocl_write_buffers(sotl_device_t *dev);

void ocl_one_step_move(sotl_device_t *dev);

void ocl_updateModelFromHost(sotl_device_t *dev);

void ocl_finalize(void);

#define cur_pos_buf(dev) (dev->pos_buffer + (dev)->cur_pb)
#define cur_spd_buf(dev) (dev->speed_buffer + (dev)->cur_sb)

#define alt_pos_buf(dev) (dev->pos_buffer + 1 - (dev)->cur_pb)
#define alt_spd_buf(dev) (dev->speed_buffer + 1 - (dev)->cur_sb)

extern cl_mem vbo_buffer;
extern cl_mem model_buffer;

#endif
