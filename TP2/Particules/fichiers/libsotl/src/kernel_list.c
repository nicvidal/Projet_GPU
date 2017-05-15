
#include "default_defines.h"
#include "kernel_list.h"

static char *kernels_name[KERNEL_TAB_SIZE] = {
  "gravity", // Gravity
  "eating", // Eating
  "growing", // Growing
  "reset_int_buffer", // reset_int
  "box_count_all_atoms", // box count_all
  "null_kernel", // box_count (NOT USED)
  "null_kernel", // scan (TODO)
  "null_kernel", // scan2 (TODO)
  "copy_buffer", // copy
  "box_sort_all_atoms", // box_sort_all
  "null_kernel", // box_sort (NOT_USED)
  "box_force", // box_force
  "lennard_jones", // force
  "border_collision",  // bounce
  "update_position", // update_position
#ifdef _SPHERE_MODE_
  "update_vertices", // update_vertices
#else
  "update_vertice", // update_vertices
#endif

  "zero_speed", // zero_speed
  "atom_collision", // collision
  "null_kernel", // NULL 
};



char *kernel_name(unsigned kernel_num)
{
  return kernels_name[kernel_num];
}
