
#ifndef SEQ_H
#define SEQ_H

#include "device.h"

void seq_init (sotl_device_t *dev);
void seq_alloc_buffers (sotl_device_t *dev);
void seq_finalize (sotl_device_t *dev);

void seq_one_step_move (sotl_device_t *dev);


#endif
