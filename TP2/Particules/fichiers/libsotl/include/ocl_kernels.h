
#ifndef OCL_KERNELS_H
#define OCL_KERNELS_H

#include <stdio.h>

#include "device.h"
#include "cl.h"



/* kernel functions */
void border_collision (sotl_device_t *dev);
void update_position(sotl_device_t *dev);
void zero_speed_kernel (sotl_device_t *dev);
void atom_collision (sotl_device_t *dev);
void gravity (sotl_device_t *dev);
void update_vertices (sotl_device_t *dev);
#ifdef _SPHERE_MODE_
void eating_pacman (sotl_device_t *dev);
void growing_ghost (sotl_device_t *dev);
#endif

void copy_int_buffer(sotl_device_t *dev, cl_mem *dst_buf, cl_mem *src_buf,
                     const unsigned nb_elems);
void reset_int_buffer(sotl_device_t *dev, cl_mem *buffer, const unsigned begin,
                      const unsigned end);
void reset_box_buffer(sotl_device_t *dev);
void copy_box_buffer(sotl_device_t *dev);
void scan(sotl_device_t *dev, const unsigned begin, const unsigned end);

void box_count_all_atoms(sotl_device_t *dev, const unsigned begin,
                         const unsigned end);

void box_count_own_atoms(sotl_device_t *dev, const unsigned begin,
                         const unsigned end);

void box_sort_all_atoms(sotl_device_t *dev, const unsigned begin,
                        const unsigned end);

void box_sort_own_atoms(sotl_device_t *dev, const unsigned begin,
                        const unsigned end);

void n2_lennard_jones (sotl_device_t *dev);
void box_lennard_jones(sotl_device_t *dev, const unsigned begin,
                       const unsigned end);

void null_kernel (sotl_device_t *dev);

#endif
