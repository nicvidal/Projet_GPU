/* DEVICE_H */
#ifndef DEVICE_H
#define DEVICE_H

#include <stdbool.h>

#include "atom.h"
#include "default_defines.h"
#include "domain.h"
#include "kernel_list.h"
#include "cl.h"

typedef enum {
  SOTL_COMPUTE_OCL,
  SOTL_COMPUTE_SEQ,
  SOTL_COMPUTE_OMP
} sotl_compute_t;

typedef struct {
  cl_platform_id id;
  char *name;
  char *vendor;
  unsigned selected;
} sotl_platform_t;

typedef struct {
  sotl_compute_t compute;
  cl_device_id id;              // OpenCL device id
  cl_device_type type;          // type (CPU, GPU, OTHER)
  char *name;
  unsigned max_workgroup_size;
  unsigned tile_size;           // Preferred tile (= workgroup) size
  unsigned slide_steps;         // Preferred slide steps (when SLIDE is defined)
  cl_ulong mem_size;            // Total memory on device
  sotl_platform_t *platform;
  cl_context context;
  cl_program program;
  cl_kernel kernel[KERNEL_TAB_SIZE];
  cl_command_queue queue;
  bool selected;                // Device is enabled
  bool display;                 // Device also serves as display device (for OpenGL rendering)
  sotl_domain_t domain;         // Bounds of simulation domain
  sotl_atom_set_t atom_set;
  unsigned long mem_allocated;  // Total mount of memory used by OpenCL buffers
  cl_event prof_events[KERNEL_TAB_SIZE];
  bool prof_used[KERNEL_TAB_SIZE];
  unsigned cur_pb;              // Current position buffer (0/1)
  unsigned cur_sb;              // Current speed buffer (0/1)
  cl_mem pos_buffer[2];
  cl_mem speed_buffer[2];
  cl_mem box_buffer;
  cl_mem calc_offset_buffer;
  cl_mem min_buffer;
  cl_mem max_buffer;
  cl_mem fake_min_buffer;
  cl_mem fake_max_buffer;
  cl_mem domain_buffer;
} sotl_device_t;

/**
 * Finalize a device (ie. free memory objects and so on)
 */
void device_finalize(sotl_device_t *dev);

/**
 * Create buffer objects on the given device.
 */
void device_create_buffers(sotl_device_t *dev);

/**
 * Write buffer objects on the given device.
 */
void device_write_buffers(sotl_device_t *dev);

/**
 * Init ghosts.
 */
void device_init_ghosts(sotl_device_t *dev);

/**
 * Read buffer objects on the given device.
 */
void device_read_buffers(sotl_device_t *dev);

/**
 * Read back atom positions.
 */
void device_read_back_pos(sotl_device_t *dev, calc_t *pos_x, calc_t *pos_y,
                          calc_t *pos_z);

/**
 * Read back atom speeds.
 */
void device_read_back_spd(sotl_device_t *dev, calc_t *spd_x, calc_t *spd_y,
                          calc_t *spd_z);

/**
 * Create kernel objects on the given device.
 */
void cl_create_kernels(sotl_device_t *dev);

/**
 * Do one step move on the given device.
 */
void device_one_step_move(sotl_device_t *dev);

/**
 * Get number of atoms on left border (only used in multi devices).
 */
unsigned device_get_natoms_left(const sotl_device_t *dev);

/**
 * Get number of atoms on right border (only used in multi devices).
 */
unsigned device_get_natoms_right(const sotl_device_t *dev);

/**
 * Get total number of atoms by reading the box buffer.
 *
 * This function must be called after scan kernel.
 */
unsigned device_get_natoms(const sotl_device_t *dev);

sotl_device_t *device_get_prev(sotl_device_t *dev);

sotl_device_t *device_get_next(sotl_device_t *dev);

extern sotl_platform_t *sotl_platforms[];
extern unsigned sotl_nb_platforms;
extern sotl_device_t *sotl_devices[];
extern unsigned sotl_nb_devices;

#endif
