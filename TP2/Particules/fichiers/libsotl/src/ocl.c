#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_LIBGL
#ifdef __APPLE__
#include <OpenGL/CGLContext.h>
#include <OpenGL/CGLCurrent.h>
#else
#include </usr/include/GL/glx.h>
#endif

#include "vbo.h"
#endif

#include "global_definitions.h"
#include "default_defines.h"
#include "ocl.h"
#include "ocl_kernels.h"
#include "atom.h"
#include "window.h"
#include "sotl.h"
#include "util.h"

#ifndef PROGRAM_NAME
#error "You have to define PROGRAM_NAME"
#endif

#define OPENCL_PROG_MAX_STRING_SIZE_OPTIONS 2048
#define OPENCL_PROG_STRING_OPTIONS "-DSCAN_WG_SIZE=%d -DTILE_SIZE_BOX_OFFSET=%d -DUSE_DOUBLE=%d"
#define OPENCL_PROG_PARAM_OPTIONS SCAN_WG_SIZE, TILE_SIZE_BOX_OFFSET, USE_DOUBLE

#define LENNARD_STRING "-DLENNARD_SIGMA=%.10f -DLENNARD_EPSILON=%.10f -DLENNARD_CUTOFF=%.10f -DBOX_SIZE_INV=%.10f -DLENNARD_SQUARED_CUTOFF=%.10f"
#define LENNARD_PARAM  (double)LENNARD_SIGMA,LENNARD_EPSILON,LENNARD_CUTOFF,BOX_SIZE_INV,LENNARD_SQUARED_CUTOFF

#ifdef HAVE_LIBGL
cl_mem vbo_buffer;
cl_mem model_buffer;
#endif

#ifdef HAVE_LIBGL
void ocl_acquire(sotl_device_t *dev)
{
  cl_int err;                            // error code returned from api calls

    glFinish();

    if (dev->compute == SOTL_COMPUTE_OCL) {
      err = clEnqueueAcquireGLObjects(dev->queue, 1, &vbo_buffer, 0, NULL, NULL);
      check(err, "Failed to acquire lock");
    }
}

void ocl_release(sotl_device_t *dev)
{
  cl_int err;                            // error code returned from api calls

  if (dev->compute == SOTL_COMPUTE_OCL) {
    err = clEnqueueReleaseGLObjects(dev->queue, 1, &vbo_buffer, 0, NULL, NULL);
    check(err, "Failed to release lock");

    clFinish(dev->queue);
  }
}
#endif

void ocl_init(sotl_device_t *dev)
{
  cl_int err = 0;

  // Create context
  //
#ifdef HAVE_LIBGL
  if(dev->display) {
#ifdef __APPLE__
    CGLContextObj cgl_context = CGLGetCurrentContext ();
    CGLShareGroupObj sharegroup = CGLGetShareGroup (cgl_context);
    cl_context_properties properties[] = {
      CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
      (cl_context_properties) sharegroup,
      0
    };

#else
    cl_context_properties properties[] = {
      CL_GL_CONTEXT_KHR,
      (cl_context_properties) glXGetCurrentContext (),
      CL_GLX_DISPLAY_KHR,
      (cl_context_properties) glXGetCurrentDisplay (),
      CL_CONTEXT_PLATFORM, (cl_context_properties) dev->platform->id,
      0
    };
#endif

    dev->context = clCreateContext (properties, 1, &dev->id, NULL, NULL, &err);
  } else
#endif
    {
      dev->context = clCreateContext (0, 1, &dev->id, NULL, NULL, &err);
    }
    check (err, "Failed to create compute context \n");

    // Load program source
    // 
    const char *opencl_prog;
    opencl_prog = file_get_contents(PROGRAM_NAME);
    if (!opencl_prog) {
        sotl_log(CRITICAL, "Failed to read contents of the OpenCL program '%s'.\n", PROGRAM_NAME);
    }

    // Build program
    // 
    dev->program = clCreateProgramWithSource (dev->context, 1, &opencl_prog, NULL, &err);
    check (err, "Failed to create program");

    char options[OPENCL_PROG_MAX_STRING_SIZE_OPTIONS];

    // Tile size of 32 works better on Xeon/Xeon Phi
    //
    if(dev->type != CL_DEVICE_TYPE_GPU)
      dev->tile_size = 32;
    else
      dev->tile_size = TILE_SIZE;

#ifdef SLIDE
      dev->slide_steps = SLIDE;
#else
      dev->slide_steps = 1;
#endif

    sprintf (options,
	     OPENCL_BUILD_OPTIONS
	     "-DTILE_SIZE=%d "
	     "-DSUBCELL=%d "
             "-DDELTA_T=%.10f "
#ifdef SLIDE
	     "-DSLIDE=%d "
#endif
	     OPENCL_PROG_STRING_OPTIONS " -I "OCL_INCLUDE " " 
	     LENNARD_STRING,
	     dev->tile_size,
	     SUBCELL,
             1.0f,
#ifdef SLIDE
	     dev->slide_steps,
#endif
	     OPENCL_PROG_PARAM_OPTIONS,
	     LENNARD_PARAM);

    // If not a GPU, do not use Tiling
    //
    if (dev->type != CL_DEVICE_TYPE_GPU)
      strcat (options, " -DNO_LOCAL_MEM");

#ifdef TILE_CACHE
    // On GPU, use a cache of TILES
    //
    if (dev->type == CL_DEVICE_TYPE_GPU)
      strcat (options, " -DTILE_CACHE");
#endif

    // Multi-accelerators configuration
    //
    if (sotl_have_multi())
      strcat (options, " -DHAVE_MULTI");

#ifdef FORCE_N_UPDATE
    if (!sotl_have_multi())
      strcat (options, " -DFORCE_N_UPDATE");
#endif

#ifdef TORUS
    strcat (options, " -DXY_TORUS -DZ_TORUS");
#endif

#ifdef XEON_VECTORIZATION
    strcat (options, " -DXEON_VECTORIZATION");
#endif

#if defined(__APPLE__)
    strcat(options, " -DHAVE_PRINTF");
#endif

    if (sotl_verbose)
      sotl_log(INFO, "--- Compiler flags ---\n%s\n----------------------\n",
               options);

    err = clBuildProgram (dev->program, 0, NULL, options, NULL, NULL);
    if(sotl_verbose || err != CL_SUCCESS) {
      size_t len;

      // Display compiler error log
      // 
      clGetProgramBuildInfo (dev->program, dev->id,
			     CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
        {
	  char buffer[len + 1];

	  clGetProgramBuildInfo (dev->program, dev->id,
				 CL_PROGRAM_BUILD_LOG,
				 sizeof (buffer), buffer, NULL);
	  sotl_log(INFO, "---- Compiler log ----\n%s\n----------------------\n",
		   buffer);
        }
        check (err, "Failed to build program");
    }

    // Create an OpenCL command queue
    //
    dev->queue = clCreateCommandQueue (dev->context, dev->id,
                CL_QUEUE_PROFILING_ENABLE, &err);
    check (err, "Failed to create a command queue");

    cl_create_kernels(dev);
}


void ocl_alloc_buffers (sotl_device_t *dev)
{
  device_create_buffers(dev);

  if (sotl_verbose)
    sotl_log(INFO, "%.2f MB of memory allocated for %d atoms on device [%s]\n",
	     dev->mem_allocated / (1024.0 * 1024.0),
             dev->atom_set.natoms, dev->name);
}

void ocl_write_buffers(sotl_device_t *dev)
{
    device_write_buffers(dev);

    if (sotl_have_multi()) {
        /* In multi devices, we add ghosts at the beginning in order to
         * have better performance. */
        device_init_ghosts(dev);
    }
}

void ocl_one_step_move(sotl_device_t *dev)
{
  unsigned begin = atom_set_begin(&dev->atom_set);
  unsigned end   = atom_set_end(&dev->atom_set);

  if (gravity_enabled)
    gravity (dev);

#ifdef _SPHERE_MODE_
  if (eating_enabled)
    eating_pacman (dev);

  if (growing_enabled)
    growing_ghost (dev);
#endif

  if (force_enabled) {

    if (is_box_mode) {
      reset_box_buffer(dev);
      box_count_all_atoms(dev, begin, end);

      // Calc boxes offsets
      scan(dev, 0, dev->domain.total_boxes + 1);

      // Sort atoms in boxes
      {
        copy_box_buffer(dev);

	// Sort
	box_sort_all_atoms(dev, begin, end);

        // box_sort_all_atoms used alternate pos & speed buffer, so we should switch...
	dev->cur_pb = 1 - dev->cur_pb;
	dev->cur_sb = 1 - dev->cur_sb;
      }

      /* Compute potential */
      box_lennard_jones(dev, begin, end);

    } else { // !BOX_MODE

      // Classic n^2 compute force version
      n2_lennard_jones (dev);
    }

  }
  
  if(detect_collision)
    atom_collision (dev);

  if(borders_enabled)
    border_collision (dev);

  update_position (dev);

#ifdef HAVE_LIBGL
  if (dev->display)
    update_vertices (dev);
#endif
}


#ifdef HAVE_LIBGL

void ocl_updateModelFromHost(sotl_device_t *dev)
{
  cl_int err;

  // Transfer model from Host to Accelerator
  //
  err = clEnqueueWriteBuffer(dev->queue, model_buffer, CL_TRUE, 0,
			     2 * vertices_per_atom * 3 * sizeof(float), vertex_model, 0, NULL, NULL);
  check(err, "Failed to write to model_buffer array");

  clFinish(dev->queue);

  update_vertices(dev);
}

#endif

void ocl_finalize (void)
{
#ifdef HAVE_LIBGL
  if(sotl_display) {
    clReleaseMemObject (vbo_buffer);
    clReleaseMemObject (model_buffer);
  }
#endif


  for(unsigned d = 0; d < sotl_nb_devices; d++) {
    sotl_device_t * const dev = sotl_devices[d];
    device_finalize(dev);
  }
}
