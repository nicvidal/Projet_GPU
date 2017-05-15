#include "ocl_kernels.h"

#include <stdio.h>

#include "default_defines.h"
#include "device.h"
#include "kernel_list.h"
#include "ocl.h"
#ifdef HAVE_LIBGL
#include "vbo.h"
#endif
#include "profiling.h"

void copy_int_buffer(sotl_device_t *dev, cl_mem *dst_buf, cl_mem *src_buf,
                     const unsigned nb_elems)
{
    int k = KERNEL_COPY_BUFFER;
    cl_int err = CL_SUCCESS;

    if (dev->type == CL_DEVICE_TYPE_GPU) {
        /* Using clEnqueueCopyBuffer() instead of our own copy buffer kernel
         * improves performance on GPU devices like NVIDIA. */
        err = clEnqueueCopyBuffer(dev->queue, *src_buf, *dst_buf, 0, 0,
                                  nb_elems * sizeof(int), 0, NULL, prof_event_ptr(dev,k));
        check(err, "Failed to copy buffer using clEnqueueCopyBuffer().\n");
    } else {
        /* Using our own copy buffer kernel is better on other devices like
         * Intel Xeon (Phi) which seems to have a bad implementation of
         * clEnqueueCopyBuffer(). */
        err |= clSetKernelArg(dev->kernel[k], 0, sizeof(cl_mem), dst_buf);
        err |= clSetKernelArg(dev->kernel[k], 1, sizeof(cl_mem), src_buf);
        err |= clSetKernelArg(dev->kernel[k], 2, sizeof(nb_elems), &nb_elems);
        check(err, "Failed to set kernel arguments: %s.\n", kernel_name(k));

        size_t local  = MIN(dev->tile_size, dev->max_workgroup_size);
        size_t global = ROUND(nb_elems);

        err = clEnqueueNDRangeKernel(dev->queue, dev->kernel[k], 1, NULL,
                                     &global, &local, 0, NULL,
                                     prof_event_ptr(dev,k));
        check(err, "Failed to exec kernel: %s.\n", kernel_name(k));
    }
}

void copy_box_buffer(sotl_device_t *dev)
{
    copy_int_buffer(dev, &dev->calc_offset_buffer, &dev->box_buffer,
                    dev->domain.total_boxes + 1);
}


void border_collision (sotl_device_t *dev)
{
    calc_t radius = ATOM_RADIUS;	

    // Set the arguments to our compute kernel
    int k = KERNEL_BORDER;
    int err = CL_SUCCESS;

    err |= clSetKernelArg (dev->kernel[k], 0, sizeof (cl_mem), cur_pos_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 1, sizeof (cl_mem), cur_spd_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 2, sizeof (cl_mem), &dev->min_buffer);
    err |= clSetKernelArg (dev->kernel[k], 3, sizeof (cl_mem), &dev->max_buffer);
    err |= clSetKernelArg (dev->kernel[k], 4, sizeof (calc_t), &radius);
    err |= clSetKernelArg (dev->kernel[k], 5, sizeof (dev->atom_set.natoms), &dev->atom_set.natoms);
    err |= clSetKernelArg (dev->kernel[k], 6, sizeof (dev->atom_set.offset), &dev->atom_set.offset);
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    size_t global = 3 * dev->atom_set.offset; 
    size_t local = MIN(dev->tile_size, dev->max_workgroup_size);

    err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
				  NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

void update_position(sotl_device_t *dev)
{
    unsigned offset = atom_set_offset(&dev->atom_set);
    size_t global, local;

    int k = KERNEL_UPDATE_POSTION;
    int err = CL_SUCCESS;

    err |= clSetKernelArg (dev->kernel[k], 0, sizeof (cl_mem), cur_pos_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 1, sizeof (cl_mem), cur_spd_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 2, sizeof (cl_mem), &dev->min_buffer);
    err |= clSetKernelArg (dev->kernel[k], 3, sizeof (cl_mem), &dev->max_buffer);
    err |= clSetKernelArg (dev->kernel[k], 4, sizeof (offset), &offset);
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    global = dev->atom_set.offset;     // One thread per atom, rounded
    local = MIN(dev->tile_size, dev->max_workgroup_size);

    err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
				  NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

void zero_speed_kernel(sotl_device_t *dev)
{
    size_t global, local;

    int k = KERNEL_ZERO_SPEED;
    int err = CL_SUCCESS;

    err |= clSetKernelArg (dev->kernel[k], 0, sizeof (cl_mem), cur_spd_buf(dev));
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    global = 3 * dev->atom_set.offset;
    local = MIN(dev->tile_size, dev->max_workgroup_size);

    err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL,
				  &global, &local, 0, NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

void atom_collision (sotl_device_t *dev)
{
    size_t global, local;
    calc_t radius = ATOM_RADIUS;	

    // Set the arguments to our compute kernel
    int k = KERNEL_COLLISION;

    int err = CL_SUCCESS;
    err |= clSetKernelArg (dev->kernel[k], 0, sizeof(cl_mem), cur_pos_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 1, sizeof(cl_mem), cur_spd_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 2, sizeof(radius), &radius);
    err |= clSetKernelArg (dev->kernel[k], 3, sizeof(dev->atom_set.natoms), &dev->atom_set.natoms);
    err |= clSetKernelArg (dev->kernel[k], 4, sizeof(dev->atom_set.offset), &dev->atom_set.offset);
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    global = ROUND(dev->atom_set.natoms);	      // One thread per atom
    local = MIN(dev->tile_size, dev->max_workgroup_size);

    err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL,
				  &global, &local, 0, NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

void n2_lennard_jones (sotl_device_t *dev)
{
    size_t global, local;

    // Set the arguments to our compute kernel
    int k = KERNEL_FORCE_N2;

    int err = CL_SUCCESS;
    err |= clSetKernelArg (dev->kernel[k], 0, sizeof(cl_mem), cur_pos_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 1, sizeof(cl_mem), cur_spd_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 2, sizeof(dev->atom_set.natoms), &dev->atom_set.natoms);
    err |= clSetKernelArg (dev->kernel[k], 3, sizeof(dev->atom_set.offset), &dev->atom_set.offset);
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    local = dev->tile_size;	
    global = ROUND (dev->atom_set.natoms);

    err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
				  NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

extern float normalized_vert[3];

void gravity (sotl_device_t *dev)
{
    size_t global, local;
    const calc_t g = 0.004;
    calc_t gx = g * normalized_vert[0],
      gy = g * normalized_vert[1],
      gz = g * normalized_vert[2];

    int k = KERNEL_GRAVITY;

    // Set the arguments to our compute kernel
    int err = CL_SUCCESS;
    err |= clSetKernelArg (dev->kernel[k], 0, sizeof(cl_mem), cur_spd_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 1, sizeof(calc_t), &gx);
    err |= clSetKernelArg (dev->kernel[k], 2, sizeof(calc_t), &gy);
    err |= clSetKernelArg (dev->kernel[k], 3, sizeof(calc_t), &gz);
    err |= clSetKernelArg (dev->kernel[k], 4, sizeof(dev->atom_set.natoms), &dev->atom_set.natoms);
    err |= clSetKernelArg (dev->kernel[k], 5, sizeof(dev->atom_set.offset), &dev->atom_set.offset);
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    global = ROUND(dev->atom_set.natoms);             // One thread per atom
    local = MIN(dev->tile_size, dev->max_workgroup_size);

    err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL,
				  &global, &local, 0, NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

void reset_int_buffer(sotl_device_t *dev, cl_mem *buffer, const unsigned begin,
                      const unsigned end)
{
    size_t global, local;
    cl_int err;
    int k = KERNEL_RESET_BOXES;

    err  = clSetKernelArg (dev->kernel[k], 0, sizeof(cl_mem),  buffer);
    err  = clSetKernelArg (dev->kernel[k], 1, sizeof(begin),  &begin);
    err  = clSetKernelArg (dev->kernel[k], 2, sizeof(end),  &end);
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    global = ROUND(end) - (begin & (~(dev->tile_size - 1)));
    local = MIN(dev->tile_size, dev->max_workgroup_size);

    clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
			    NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

void reset_box_buffer(sotl_device_t *dev)
{
    reset_int_buffer(dev, &dev->box_buffer, 0, dev->domain.total_boxes + 1);
}

static void box_count(sotl_device_t *dev, const unsigned begin,
                      const unsigned end, const int k)
{
    size_t global, local;
    int err = CL_SUCCESS;
    unsigned offset = atom_set_offset(&dev->atom_set);

    err |= clSetKernelArg (dev->kernel[k], 0, sizeof(cl_mem), cur_pos_buf(dev));
    err |= clSetKernelArg (dev->kernel[k], 1, sizeof(cl_mem), &dev->box_buffer);
    err |= clSetKernelArg (dev->kernel[k], 2, sizeof (cl_mem), &dev->fake_min_buffer);
    err |= clSetKernelArg (dev->kernel[k], 3, sizeof(cl_mem), &dev->domain_buffer);
    err |= clSetKernelArg (dev->kernel[k], 4, sizeof(offset), &offset);
    err |= clSetKernelArg (dev->kernel[k], 5, sizeof(begin), &begin);
    err |= clSetKernelArg (dev->kernel[k], 6, sizeof(end), &end);
	
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    global = ROUND(end) - (begin & (~(dev->tile_size - 1)));
    local = MIN(dev->tile_size, dev->max_workgroup_size);

    clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
			    NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

void box_count_all_atoms(sotl_device_t *dev, const unsigned begin,
                         const unsigned end)
{
    box_count(dev, begin, end, KERNEL_BOX_COUNT_ALL_ATOMS);
}

void box_count_own_atoms(sotl_device_t *dev, const unsigned begin,
                         const unsigned end)
{
    box_count(dev, begin, end, KERNEL_BOX_COUNT_OWN_ATOMS);
}

void box_lennard_jones(sotl_device_t *dev, const unsigned begin,
                       const unsigned end)
{
    size_t global, local;
    int k = KERNEL_FORCE;
    int err = CL_SUCCESS;
    unsigned natoms;
    unsigned offset = atom_set_offset(&dev->atom_set);

    err |= clSetKernelArg(dev->kernel[k], 0,
                          sizeof(cl_mem), cur_pos_buf(dev));
    err |= clSetKernelArg(dev->kernel[k], 1,
                          sizeof(cl_mem), cur_spd_buf(dev));
    err |= clSetKernelArg(dev->kernel[k], 2,
                          sizeof(cl_mem), &dev->box_buffer);
    err |= clSetKernelArg(dev->kernel[k], 3,
                          sizeof(cl_mem), &dev->fake_min_buffer);
    err |= clSetKernelArg(dev->kernel[k], 4,
                          sizeof(cl_mem), &dev->domain_buffer);
    err |= clSetKernelArg(dev->kernel[k], 5,
                          sizeof(cl_mem), alt_pos_buf(dev));
    err |= clSetKernelArg(dev->kernel[k], 6,
                          sizeof(cl_mem), &dev->min_buffer);
    err |= clSetKernelArg(dev->kernel[k], 7,
                          sizeof(cl_mem), &dev->max_buffer);
    err |= clSetKernelArg(dev->kernel[k], 8,
                          sizeof(offset), &offset);
    err |= clSetKernelArg(dev->kernel[k], 9,
                          sizeof(begin), &begin);
    err |= clSetKernelArg(dev->kernel[k], 10,
                          sizeof(end), &end);
    check(err, "Failed to set kernel arguments: %s.\n", kernel_name(k));

    natoms = ROUND(end) - (begin & (~(dev->tile_size - 1)));
    global = ALRND(dev->slide_steps * dev->tile_size, natoms) / dev->slide_steps;
    local  = dev->tile_size;

    err = clEnqueueNDRangeKernel(dev->queue, dev->kernel[k], 1, NULL, &global,
                                 &local, 0, NULL, prof_event_ptr(dev, k));
    check(err, "Failed to exec kernel: %s.\n", kernel_name(k));
}

static void box_sort(sotl_device_t *dev, const unsigned begin,
                     const unsigned end, const int k)
{
  size_t global, local;
  unsigned offset = atom_set_offset(&dev->atom_set);

  int err = CL_SUCCESS;
  err  = clSetKernelArg (dev->kernel[k], 0, sizeof(cl_mem), cur_pos_buf(dev));
  err |= clSetKernelArg (dev->kernel[k], 1, sizeof(cl_mem), alt_pos_buf(dev));
  err |= clSetKernelArg (dev->kernel[k], 2, sizeof(cl_mem), cur_spd_buf(dev));
  err |= clSetKernelArg (dev->kernel[k], 3, sizeof(cl_mem), alt_spd_buf(dev));
  err |= clSetKernelArg (dev->kernel[k], 4, sizeof(cl_mem), &dev->calc_offset_buffer);
  err |= clSetKernelArg (dev->kernel[k], 5, sizeof(cl_mem), &dev->fake_min_buffer);
  err |= clSetKernelArg (dev->kernel[k], 6, sizeof(cl_mem), &dev->domain_buffer);
  err |= clSetKernelArg (dev->kernel[k], 7, sizeof(offset), &offset);
  err |= clSetKernelArg (dev->kernel[k], 8, sizeof(begin), &begin);
  err |= clSetKernelArg (dev->kernel[k], 9, sizeof(end), &end);
  check(err, "Failed to set kernel arguments: %s", kernel_name(k));

  global = ROUND(end) - (begin & (~(dev->tile_size - 1)));
  local = MIN(dev->tile_size, dev->max_workgroup_size);
  
  err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
				NULL, prof_event_ptr(dev,k));
  check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

void box_sort_all_atoms(sotl_device_t *dev, const unsigned begin,
                        const unsigned end)
{
    box_sort(dev, begin, end, KERNEL_BOX_SORT_ALL_ATOMS);
}

void box_sort_own_atoms(sotl_device_t *dev, const unsigned begin,
                        const unsigned end)
{
    box_sort(dev, begin, end, KERNEL_BOX_SORT_OWN_ATOMS);
}

#ifdef HAVE_LIBGL
void update_vertices (sotl_device_t *dev)
{
  int k = KERNEL_UPDATE_VERTICES;
  int err = CL_SUCCESS;

  err |= clSetKernelArg (dev->kernel[k], 0, sizeof (cl_mem), &vbo_buffer);
  err |= clSetKernelArg (dev->kernel[k], 1, sizeof (cl_mem), cur_pos_buf(dev));
  err |= clSetKernelArg (dev->kernel[k], 2, sizeof (dev->atom_set.offset), &dev->atom_set.offset);
#ifdef _SPHERE_MODE_
  err |= clSetKernelArg (dev->kernel[k], 3, sizeof(cl_mem), &model_buffer);
  err |= clSetKernelArg (dev->kernel[k], 4, sizeof(unsigned), &vertices_per_atom);
#endif
  check(err, "Failed to set kernel arguments: %s", kernel_name(k));

  size_t global = nb_vertices * 3;
  size_t local = 1;

  err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
				NULL, prof_event_ptr(dev,k));
  check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}
#endif

#define PERIOD    10
static calc_t dy = 0.01;
static calc_t factor = 1.1;
static unsigned long e_step = 0;
static unsigned long g_step = 0;

#ifdef HAVE_LIBGL
void eating_pacman(sotl_device_t *dev)
{
    size_t global;		// global domain size for our calculation
    size_t local;		// local domain size for our calculation

    if(++e_step % PERIOD == 0)
        dy *= -1;

    // Set the arguments to our compute kernel
    //
    int k = KERNEL_EATING_PACMAN;
    int err = CL_SUCCESS;
    err |= clSetKernelArg (dev->kernel[k], 0, sizeof (cl_mem), &model_buffer);
    err |= clSetKernelArg (dev->kernel[k], 1, sizeof (calc_t), &dy);
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    global = vertices_per_atom * 3; // One thread per model vertex coordinate
    local = 1;		            // Set workgroup size to 1

    err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
				  NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

void growing_ghost(sotl_device_t *dev)
{
    size_t global;		// global domain size for our calculation
    size_t local;		// local domain size for our calculation

    if(++g_step % PERIOD == 0)
        factor = 1.0 / factor;

    // Set the arguments to our compute kernel
    // 
    int k = KERNEL_GROWING_GHOST;
    int err = CL_SUCCESS;
    err |= clSetKernelArg (dev->kernel[k], 0, sizeof (cl_mem), &model_buffer);
    err |= clSetKernelArg (dev->kernel[k], 1, sizeof (calc_t), &factor);
    check(err, "Failed to set kernel arguments: %s", kernel_name(k));

    global = vertices_per_atom * 3; // One thread per model vertex coordinate
    local = 1;		            // Set workgroup size to 1

    err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
				  NULL, prof_event_ptr(dev,k));
    check(err, "Failed to exec kernel: %s\n", kernel_name(k));
}

#endif

void resetAnimation (void)
{
    dy = 0.01;
    factor = 1.05;
    e_step = g_step = 0;
}


void scan(sotl_device_t *dev, const unsigned begin, const unsigned end)
{
    // TODO
  if(begin < end) // Silly code to avoid warning
    dev = NULL;
}

void null_kernel (sotl_device_t *dev)
{
  int k = KERNEL_NULL;
  int err = CL_SUCCESS;
  size_t global = dev->tile_size;
  size_t local = dev->tile_size;

  err = clEnqueueNDRangeKernel (dev->queue, dev->kernel[k], 1, NULL, &global, &local, 0,
				NULL, prof_event_ptr(dev,k));
  check(err, "Failed to exec kernel: %s\n", kernel_name(k));

  clFinish (dev->queue);
}
