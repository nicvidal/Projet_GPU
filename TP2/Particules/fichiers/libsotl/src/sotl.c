#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <errno.h>
#include <stdarg.h>

#include "domain.h"
#include "sotl.h"
#include "atom.h"
#include "default_defines.h"
#include "global_definitions.h"
#include "device.h"
#include "ocl.h"
#include "ocl_kernels.h"
#include "seq.h"

#ifdef HAVE_OMP
#include "openmp.h"
#endif

#ifdef HAVE_LIBGL
#include "window.h"
#include "vbo.h"
#endif
#include "profiling.h"

#define MAX_PLATFORMS  5
#define MAX_DEVICES    5

// OpenCL platforms & devices
//
static cl_device_id ocl_dev[MAX_DEVICES * MAX_PLATFORMS];
static unsigned nb_ocl_devs = 0;
static cl_platform_id ocl_pf[MAX_PLATFORMS];
static unsigned nb_ocl_pfs = 0;

static unsigned first_dev[MAX_PLATFORMS];
static unsigned last_dev[MAX_PLATFORMS];

// SOTL platforms & devices
//
static sotl_platform_t all_platforms[MAX_PLATFORMS];
sotl_platform_t *sotl_platforms[MAX_PLATFORMS];
unsigned sotl_nb_platforms = 0;

static sotl_device_t all_devices[MAX_DEVICES];
sotl_device_t *sotl_devices[MAX_DEVICES * MAX_PLATFORMS];
unsigned sotl_nb_devices = 0;

static int opengl_device = -1;

static bool no_device_selected = true;

static sotl_params_t params;

sotl_params_t *get_params()
{
    return &params;
}

void sotl_log(const int type, const char *fmt, ...)
{
    va_list argptr;

    switch (type) {
        case DEBUG:
            fprintf(stderr, "[DEBUG] ");
            break;
        case INFO:
            fprintf(stderr, "[INFO] ");
            break;
        case PERF:
            fprintf(stderr, "[PERF] ");
            break;
        case WARNING:
            fprintf(stderr, "[WARNING] ");
            break;
        case ERROR:
            fprintf(stderr, "[ERROR] ");
            break;
        case CRITICAL:
            fprintf(stderr, "[CRITICAL] ");
            break;
        default:
            break;
    }

    va_start(argptr, fmt);
    vfprintf(stderr, fmt, argptr);
    va_end(argptr);

    if (type == CRITICAL) {
        fprintf(stderr, "Exit failure!\n");
        exit(1);
    }
}

int sotl_have_multi()
{
    return sotl_nb_devices > 1;
}

sotl_device_t *sotl_display_device (void)
{
  return sotl_devices[opengl_device];
}

static void sotl_discover_devices()
{
  cl_int err;
  bool no_cpu_device = true;

  // Get list of OpenCL platforms detected
  //
  err = clGetPlatformIDs (MAX_PLATFORMS, ocl_pf, &nb_ocl_pfs);

  if (err != CL_SUCCESS) {
    sotl_log(WARNING, "Failed to get OpenCL platform IDs\n");
    // Create a fake CPU platform
    nb_ocl_pfs = 0;
  }

  // Print name & vendor for each platform
  //
  for (unsigned int p = 0; p < nb_ocl_pfs; p++) {
    size_t size;
    char name[1024], vendor[1024];

    all_platforms[p].id = ocl_pf[p];
    all_platforms[p].selected = false;

    err = clGetPlatformInfo (ocl_pf[p], CL_PLATFORM_NAME, 1024, name, &size);
    if (err == -1001)
      sotl_log(CRITICAL, "Failed to find the Installable Client Driver (ICD) loader.\n");
    check (err, "Failed to get Platform Info");

    all_platforms[p].name = xmalloc (size);
    strcpy (all_platforms[p].name, name);

    err = clGetPlatformInfo (ocl_pf[p], CL_PLATFORM_VENDOR, 1024, vendor, &size);
    check (err, "Failed to get Platform Info");

    all_platforms[p].vendor = xmalloc (size);
    strcpy (all_platforms[p].vendor, vendor);

    first_dev[p] = nb_ocl_devs;

    // Get list of devices for current platform
    // 
    {
      unsigned nb_devices;

      err = clGetDeviceIDs (ocl_pf[p], CL_DEVICE_TYPE_ALL, MAX_DEVICES, 
			    ocl_dev + nb_ocl_devs, &nb_devices);
      check (err, "Failed to get OpenCL Devices' IDs");

      nb_ocl_devs += nb_devices;
      last_dev[p] = nb_ocl_devs - 1;

      {
	char name[1024];

	for (unsigned d = first_dev[p]; d <= last_dev[p]; d++) {
	  cl_device_type dtype;
	  size_t size;

	  all_devices[d].id = ocl_dev[d];
	  all_devices[d].platform = &all_platforms[p];
	  all_devices[d].selected = false;
	  all_devices[d].display = false;
	  all_devices[d].mem_allocated = 0;

	  err = clGetDeviceInfo (ocl_dev[d], CL_DEVICE_NAME, 1024, name, &size);
	  check (err, "Cannot get name of device");

	  all_devices[d].name = xmalloc(size);
	  strcpy (all_devices[d].name, name);

	  err =  clGetDeviceInfo (ocl_dev[d], CL_DEVICE_TYPE,
				  sizeof (cl_device_type), &dtype, NULL);
	  check (err, "Cannot get type of device");

	  if (dtype == CL_DEVICE_TYPE_CPU)
	    no_cpu_device = false;

	  all_devices[d].type = dtype;

#ifdef __APPLE__
	  if(dtype == CL_DEVICE_TYPE_CPU)
	    all_devices[d].max_workgroup_size = 1;
	  else
#endif
	    err = clGetDeviceInfo (ocl_dev[d], CL_DEVICE_MAX_WORK_GROUP_SIZE,
				   sizeof(size_t), &all_devices[d].max_workgroup_size, &size);
	  check (err, "Cannot get max workgroup size");

	  err = clGetDeviceInfo (ocl_dev[d], CL_DEVICE_GLOBAL_MEM_SIZE,
				 sizeof(cl_ulong), &all_devices[d].mem_size, &size);
	  check (err, "Cannot get mem size");
	}
      }
    }
  }

  // If no OpenCL CPU device was found, create a fake one to enable sequential/OpenMP mode
  //
  if (no_cpu_device) {
    unsigned p = nb_ocl_pfs++;
    unsigned d = nb_ocl_devs++;

    all_platforms[p].vendor = str_malloc ("Fake Vendor");
    all_platforms[p].name = str_malloc ("Fake Platform");
    all_platforms[p].selected = false;

    first_dev[p] = d;
    last_dev[p] = d;

    all_devices[d].platform = &all_platforms[p];
    all_devices[d].selected = false;
    all_devices[d].display = false;
    all_devices[d].mem_allocated = 0;
    all_devices[d].name = str_malloc ("Fake CPU Device");
    all_devices[d].type = CL_DEVICE_TYPE_CPU;
    all_devices[d].max_workgroup_size = 0;
    all_devices[d].mem_size = 0;
  }
}

int sotl_add_ocl_device_by_type(const sotl_device_type t)
{
    cl_device_type type;
    int found = 0;

    switch (t) {
        case SOTL_CPU :
            type = CL_DEVICE_TYPE_CPU;
            break;
        case SOTL_GPU :
            type = CL_DEVICE_TYPE_GPU;
            break;
        case SOTL_OTHER :
            type = CL_DEVICE_TYPE_ACCELERATOR;
            break;
        case SOTL_ALL :
            type = CL_DEVICE_TYPE_ALL;
            break;
        default:
            return SOTL_INVALID_DEVICE_TYPE;
            break;
    }

    for (unsigned d = 0; d < nb_ocl_devs; d++) {
      if ((type == CL_DEVICE_TYPE_ALL || all_devices[d].type == type) &&
	  all_devices[d].max_workgroup_size != 0) {
            all_devices[d].selected = true;
            all_devices[d].compute  = SOTL_COMPUTE_OCL;
            no_device_selected      = false;
            found                   = 1;
        }
    }

    if (!found)
        return SOTL_DEVICE_NOT_FOUND;
    return SOTL_SUCCESS;
}

int sotl_add_ocl_device_by_id(const unsigned dev_id)
{
  if (dev_id >= nb_ocl_devs)
    return SOTL_INVALID_DEVICE;

  // Check if device is a fake CPU device
  //
  if (all_devices[dev_id].max_workgroup_size == 0)
    return SOTL_INVALID_DEVICE_TYPE;

    all_devices[dev_id].selected = true;
    all_devices[dev_id].compute  = SOTL_COMPUTE_OCL;
    no_device_selected           = false;

    return SOTL_SUCCESS;
}

int sotl_add_seq_device_by_id(const unsigned dev_id)
{
    if (dev_id >= nb_ocl_devs)
        return SOTL_INVALID_DEVICE;

    if (all_devices[dev_id].type != CL_DEVICE_TYPE_CPU)
        return SOTL_INVALID_DEVICE_TYPE;

    all_devices[dev_id].selected = true;
    all_devices[dev_id].compute  = SOTL_COMPUTE_SEQ;
    no_device_selected           = false;

    return SOTL_SUCCESS;
}

int sotl_add_omp_device_by_id(const unsigned dev_id)
{
    if (dev_id >= nb_ocl_devs)
        return SOTL_INVALID_DEVICE;

    if (all_devices[dev_id].type != CL_DEVICE_TYPE_CPU)
        return SOTL_INVALID_DEVICE_TYPE;

    all_devices[dev_id].selected = true;
    all_devices[dev_id].compute  = SOTL_COMPUTE_OMP;
    no_device_selected           = false;

    return SOTL_SUCCESS;
}

int sotl_set_output_device(const unsigned dev_id)
{
    if (dev_id >= nb_ocl_devs)
        return SOTL_INVALID_DEVICE;

    opengl_device = dev_id;
    sotl_display  = 1;

    return SOTL_SUCCESS;
}

int sotl_get_selected_gpus()
{
    int n = 0;

    for (unsigned i = 0; i < nb_ocl_devs; i++) {
        sotl_device_t *d = &all_devices[i];

        if (d->type == CL_DEVICE_TYPE_GPU && d->selected)
            n++;
    }
    return n;
}

int sotl_get_selected_mics()
{
    int n = 0;

#define MIC_VENDOR_NAME "Intel(R) Many Integrated Core Acceleration Card"
    for (unsigned i = 0; i < nb_ocl_devs; i++) {
        sotl_device_t *d = &all_devices[i];

        if (d->type == CL_DEVICE_TYPE_CPU && d->selected
            && !strcmp(d->name, MIC_VENDOR_NAME))
            n++;
    }
#undef MIC_VENDOR_NAME
    return n;
}

static void sotl_fix_device_list (void)
{
#ifdef HAVE_LIBGL
  int output = -1;
#endif

  if (no_device_selected) {
    if (sotl_verbose)
      sotl_log(INFO, "No device specified - Selecting device 0 by default\n");
    sotl_add_ocl_device_by_id (0);
  }

  if (sotl_verbose)
    sotl_log(INFO, "Selected devices:\n");

  for (unsigned d = 0; d < nb_ocl_devs; d++) {

    if (all_devices[d].selected) {

#ifdef HAVE_LIBGL
      if (sotl_display) {
	// If no output device has been specified, take first selected device by default
	//
	if (opengl_device == -1 || (int)d == opengl_device) {
	  output = sotl_nb_devices;
	  all_devices[d].display = true;
	}
      }
#endif

      if (! all_devices[d].platform->selected) {

	all_devices[d].platform->selected = true;

	sotl_platforms[sotl_nb_platforms++] = all_devices[d].platform;
      }

      if (sotl_verbose)
        sotl_log(INFO, "%d: [%s (%s)] %s\n",
		 sotl_nb_devices,
		 all_devices[d].name, all_devices[d].platform->name,
		 all_devices[d].display ? "\t*** Output device ***" : "" );

      // Add device to the set of selected devices
      //
      sotl_devices[sotl_nb_devices++] = all_devices + d;

    }

  }

#ifdef HAVE_LIBGL
  if (sotl_display) {
    if (output == -1) {
      sotl_log(ERROR, "Output device (%d) not among selected devices!\n", opengl_device);
      exit (1);
    } else
      opengl_device = output;
  }
#endif

}

int sotl_domain_init(const double *xrange, const double *yrange,
                     const double *zrange, const double *cellbox,
                     const unsigned natoms)
{
    if (xrange[1] <= xrange[0] || yrange[1] <= yrange[0] || zrange[1] <= zrange[0])
        return SOTL_INVALID_VALUE;

    domain_init(get_global_domain(), xrange[0], yrange[0], zrange[0],
                xrange[1], yrange[1], zrange[1]);

    if (sotl_verbose)
        domain_print(get_global_domain());

    sotl_fix_device_list ();

    atom_set_init (get_global_atom_set(), natoms, natoms);

    (void)cellbox;
    return SOTL_SUCCESS;
}

int sotl_set_parameter(const unsigned int name, const void *value)
{
    switch (name) {
        /* Molecular Dynamics. */
        case MD_DELTA_T:
            params.md.delta_t = *(double *)value;
            break;
        /* Lennard Jones. */
        case LJ_SIGMA:
            params.lj.sigma = *(double *)value;
            break;
        case LJ_EPSILON:
            params.lj.epsilon = *(double *)value;
            break;
        case LJ_RCUT:
            params.lj.rcut = *(double *)value;
            break;
        default:
            return SOTL_INVALID_PARAMETER;
            break;
    }

    return SOTL_SUCCESS;
}

const void *sotl_get_parameter(const unsigned int name)
{
    void *value = NULL;

    switch (name) {
        /* Molecular Dynamics. */
        case MD_DELTA_T:
            value = (void *)&params.md.delta_t;
            break;
        /* Lennard Jones. */
        case LJ_SIGMA:
            value = (void *)&params.lj.sigma;
            break;
        case LJ_EPSILON:
            value = (void *)&params.lj.epsilon;
            break;
        case LJ_RCUT:
            value = (void *)&params.lj.rcut;
            break;
        default:
            break;
    }

    return value;
}

int sotl_sync_all_atoms(unsigned *ids, calc_t *pos_x, calc_t *pos_y,
                        calc_t *pos_z, calc_t *spd_x, calc_t *spd_y,
                        calc_t *spd_z, const unsigned natoms)
{
    unsigned total_natoms = 0;
    size_t offset;
    unsigned d;

    /* Count total number of atoms among selected devices. */
    for (d = 0; d < sotl_nb_devices; d++) {
        total_natoms += sotl_devices[d]->atom_set.natoms;
    }

    if (total_natoms != natoms)
        return SOTL_INVALID_VALUE;

    /* Read back atom positions and speeds. */
    offset = 0;
    for (d = 0; d < sotl_nb_devices; d++) {
        sotl_device_t *dev = sotl_devices[d];
        device_read_back_pos(dev, pos_x + offset, pos_y + offset,
                             pos_z + offset);
        device_read_back_spd(dev, spd_x + offset, spd_y + offset,
                             spd_z + offset);
        offset += dev->atom_set.natoms;
    }

    (void)ids; /* XXX */
    return SOTL_SUCCESS;
}

int sotl_push_pos_1()
{
    return SOTL_NOT_IMPLEMENTED;
}

int sotl_push_pos_2()
{
    return SOTL_NOT_IMPLEMENTED;
}

int sotl_compute_force()
{
    return SOTL_NOT_IMPLEMENTED;
}

int sotl_reset_force()
{
    return SOTL_NOT_IMPLEMENTED;
}

void sotl_add_atom (calc_t x, calc_t y, calc_t z, 
		    calc_t dx, calc_t dy, calc_t dz)
{
  atom_set_add (get_global_atom_set(), x, y, z, dx, dy, dz);
}

static calc_t rand_calc_t(const calc_t a, const calc_t b)
{
    calc_t r = random() / (calc_t)RAND_MAX;
    return a + (b - a) * r;
}

void sotl_add_random_atom()
{
    sotl_domain_t *d = get_global_domain();
    calc_t x, y, z, dx, dy, dz;
    calc_t spd, lat, lon;

    /* Random atom positions. */
    x = rand_calc_t(d->min_ext[0], d->max_ext[0]);
    y = rand_calc_t(d->min_ext[1], d->max_ext[1]);
    z = rand_calc_t(d->min_ext[2], d->max_ext[2]);

    /* Compute speed, latitude and longitude. */
    spd = rand_calc_t(ATOM_RADIUS * 0.05, ATOM_RADIUS * 0.2);
    lat = rand_calc_t(-M_PI / 2, M_PI / 2);
    lon = rand_calc_t(0.0, M_PI * 2);

    /* Compute atom speeds. */
    dx = cos(lon) * cos(lat) * spd;
    dy = sin(lat) * spd;
    dz = -sin(lon) * cos(lat) * spd;

    /* Add this atom to the global atom set. */
    sotl_add_atom(x, y, z, dx, dy, dz);
}

int sotl_runtime_init()
{
    int ret = 0;

    if (sotl_verbose)
        sotl_list_devices();

#ifdef HAVE_LIBGL
  if (sotl_display)
    window_opengl_init (DISPLAY_XSIZE, DISPLAY_YSIZE,
			get_global_domain()->min_ext, get_global_domain()->max_ext, 1);
#endif

  // Initialize OpenCL resources associated to device
  //
  for(unsigned d = 0; d < sotl_nb_devices; d++) {
    switch (sotl_devices[d]->compute) {
    case SOTL_COMPUTE_OCL :
      ocl_init (sotl_devices[d]);
      break;
    case SOTL_COMPUTE_SEQ :
      seq_init (sotl_devices[d]);
      break;
    case SOTL_COMPUTE_OMP :
#ifdef HAVE_OMP
      omp_init (sotl_devices[d]);
      break;
#else
      sotl_log(ERROR, "Application was not compiled with OpenMP support\n");
      return -1;
#endif
    default :
      sotl_log(ERROR, "Undefined compute method %d\n", sotl_devices[d]->compute);
      return -1;
    }
  }

  if (sotl_have_multi()) {
     /* Sort the global atom set along z-axis using a heap sort. */
    sotl_log(DEBUG, "Trying to sort atoms along z-axis...\n");
    atom_set_sort(get_global_atom_set());
    sotl_log(DEBUG, "Done.\n");
  }

  if(sotl_verbose)
    sotl_log(INFO, "Total #atoms: %d\n", get_global_atom_set()->natoms);

  if(sotl_have_multi()) {
    sotl_log(WARNING, "Multiple devices is NOT really supported.\n");
  }

#ifdef HAVE_LIBGL
  if (sotl_display) {
    // Initialize arrays of vertices and triangles
    //
    vbo_initialize (get_global_atom_set()->natoms);
    atom_build (get_global_atom_set()->natoms, &get_global_atom_set()->pos);
    vbo_build (sotl_devices[opengl_device]);
  }
#endif

  if (sotl_have_multi()) {
    /* Split the global domain into the number of available devices. */
    domain_split(get_global_domain(), sotl_nb_devices);
  }

  // Distribute domain among available devices
  //
  // TODO: if multiple devices are selected, we must:
  //        - sort atoms along lexical order (z, y, x)
  //        - Partition domain along the z axis
  //        - Balance the atoms wrt the volume partition
  //
  for (unsigned d = 0; d < sotl_nb_devices; d++) {
    // FIXME: device shares pos & speed buffer with get_global_atom_set()!! That's bad!
    //
   if (sotl_have_multi()) {
      sotl_devices[d]->domain = *get_global_domain()->subdomains[d];
      sotl_devices[d]->atom_set = *get_global_domain()->subdomains[d]->atom_set;
   } else {
      sotl_devices[d]->domain = *get_global_domain();
      sotl_devices[d]->atom_set = *get_global_atom_set();
    }

   switch (sotl_devices[d]->compute) {
   case SOTL_COMPUTE_OCL :
     ocl_alloc_buffers (sotl_devices[d]);
     ocl_write_buffers (sotl_devices[d]);
     break;
   case SOTL_COMPUTE_SEQ :
     seq_alloc_buffers (sotl_devices[d]);
     break;
#ifdef HAVE_OMP
   case SOTL_COMPUTE_OMP :
     omp_alloc_buffers (sotl_devices[d]);
     break;
#endif
   default :
     sotl_log(ERROR, "Undefined compute method %d\n", sotl_devices[d]->compute);
     return -1;
     break;
   }
  }

#ifndef _SPHERE_MODE_
  // Yes We Can free pos & speed buffers!!  (well, hum, not in
  // Sphere mode because change_skin needs to rebuild everything
  // from main memory). Also, it is not possible in Dump mode,
  // because sotl uses the global buffer to get back positions and
  // dump them on disk
  if (!sotl_dump) {
    for (unsigned d = 0; d < sotl_nb_devices; d++) {
      if (sotl_devices[d]->compute != SOTL_COMPUTE_OCL) {
        /* Memory allocated by the global atom set on the CPU
         * may be used by other versions like sequential or OpenMP. */
        break;
      }
      atom_set_free(get_global_atom_set());
    }
  }
#endif

    return ret;
}

static void sotl_one_iteration (void)
{
  for (unsigned d = 0; d < sotl_nb_devices; d++)
    device_one_step_move (sotl_devices[d]);
}

#ifdef HAVE_LIBGL
// Called from within gluMainLoop, so additional sync is mandatory
//
static void sotl_one_step (void)
{
  ocl_acquire (sotl_devices[opengl_device]);

  if (move_enabled)
    sotl_one_iteration ();

  ocl_release (sotl_devices[opengl_device]);
}
#endif

void sotl_main_loop (unsigned nb_iter)
{
#ifdef HAVE_LIBGL

  if (sotl_display) {
    window_main_loop (sotl_one_step, nb_iter);
    return;
  }

#endif

  // Default values
  move_enabled = 1;
  force_enabled = 1;
  borders_enabled = 0;


#ifdef PROFILING
  struct timeval t1,t2;
#endif
#ifdef DEBUG
  cl_ulong start, end;
#endif

#ifdef PROFILING
  for (unsigned d = 0; d < sotl_nb_devices; d++)
    profiling_init(sotl_devices[d]);

#ifdef DEBUG
  {
    null_kernel (sotl_devices[0]);
    clGetEventProfilingInfo(sotl_devices[0]->prof_events[KERNEL_NULL], CL_PROFILING_COMMAND_END,
			    sizeof(cl_ulong), &start, NULL);

  }
#endif
  gettimeofday (&t1,NULL);
#endif

  for (unsigned i = 0; i < nb_iter; ++i)
    sotl_one_iteration ();

  for (unsigned d = 0; d < sotl_nb_devices; d++)
    if (sotl_devices[d]->compute == SOTL_COMPUTE_OCL)
      clFinish (sotl_devices[d]->queue);

#ifdef PROFILING
  gettimeofday (&t2,NULL);
  sotl_log(PERF, "   #atoms\ttime/i (µs)\tMatoms/i/s\n");
  sotl_log(PERF, "%9d\t%11.0lf\t%10.1lf\n",
	   get_global_atom_set ()->natoms,
	   ((double)TIME_DIFF(t1, t2)) / nb_iter,
	   (double)get_global_atom_set ()->natoms/(((double)TIME_DIFF(t1, t2)) / (nb_iter)));
	   
#ifdef DEBUG
  {
    null_kernel (sotl_devices[0]);
    clGetEventProfilingInfo (sotl_devices[0]->prof_events[KERNEL_NULL], CL_PROFILING_COMMAND_START,
			     sizeof(cl_ulong), &end, NULL);

  }
  sotl_log(DEBUG, "OpenCL pretends %f µs ;)\n", (end - start) / nb_iter * 1.0e-3f);
#endif

  for (unsigned d = 0; d < sotl_nb_devices; d++)
    profiling_finalize (sotl_devices[d]);

#endif
}

#define CONF_FORMAT_NUM_ATOMS   "%09d"
#define CONF_FORMAT_DOMAIN_SIZE "%f %f"
#define CONF_FORMAT_SPEED       "%d"
#define CONF_FORMAT_ATOM        "%f %f %f"

static void sotl_write_file_header(FILE *f, const sotl_domain_t *d,
                                   const sotl_atom_set_t *s)
{
    // Total number of atoms in the set.
    fprintf (f, CONF_FORMAT_NUM_ATOMS"\n", s->natoms);

    // Dimensions of the domain.
    for (int i = 0; i < 3; i++) {
        fprintf (f, CONF_FORMAT_DOMAIN_SIZE"\n", d->min_ext[i], d->max_ext[i]);
    }

    // Speed information.
    fprintf (f, CONF_FORMAT_SPEED"\n", 1);
}

static void sotl_write_file_body(FILE *f, const sotl_atom_set_t *s)
{
    for (unsigned i = 0; i < s->natoms; i++) {
        // Atom positions and speeds.
        fprintf (f, CONF_FORMAT_ATOM"\n", s->pos.x[i], s->pos.y[i],
		 s->pos.z[i]);
        fprintf (f, CONF_FORMAT_ATOM"\n", s->speed.dx[i], s->speed.dy[i],
		 s->speed.dz[i]);
    }
}

int sotl_dump_positions(const char *filename)
{
    FILE *f;

    if (!(f = fopen (filename, "w")))
        return -errno;

    sotl_write_file_header (f, get_global_domain(), get_global_atom_set());

    for (unsigned d = 0; d < sotl_nb_devices; d++) {
        sotl_device_t *dev = sotl_devices[d];

        /* Read back positions and spees from global memory. */
        device_read_buffers (dev);

        sotl_write_file_body (f, &dev->atom_set);
    }

    if (fclose(f) < 0)
        return -errno;
    return 0;
}

static void sotl_default_params_init()
{
    params.lj.sigma   = LJ_SIGMA_DEFAULT_VALUE;
    params.lj.epsilon = LJ_EPSILON_DEFAULT_VALUE;
    params.lj.rcut    = LJ_RCUT_DEFAULT_VALUE;
}

int sotl_init()
{
    /* Find all available OpenCL devices including CPU, GPU and accelerators. */
    sotl_discover_devices ();

    sotl_default_params_init();
    return SOTL_SUCCESS;
}

void sotl_list_devices()
{
    sotl_log(INFO, "%d OpenCL platforms detected\n", nb_ocl_pfs);
    for (unsigned p = 0; p < nb_ocl_pfs; p++) {
        sotl_log(INFO, "Platform %d: %s (%s)\n",
                 p, all_platforms[p].name, all_platforms[p].vendor);

        for (unsigned d = 0; d < nb_ocl_devs; d++) {
            if (all_devices[d].platform != &all_platforms[p])
                continue;

            sotl_log(INFO, "--- Device %d : %s [%s] (mem size: %.2fGB, max wg: %d)\n", d,
		     (all_devices[d].type == CL_DEVICE_TYPE_GPU) ? "GPU" : "CPU",
                     all_devices[d].name,
		     all_devices[d].mem_size / (1024.0 * 1024.0 * 1024.0),
		     all_devices[d].max_workgroup_size);
        }
    }
}

void sotl_enable_verbose()
{
    sotl_verbose = true;
}

void sotl_enable_dump()
{
    sotl_dump = 1;
}

void sotl_finalize()
{
    /* Dump atom positions to disk. */
    if (sotl_dump) {
        if (sotl_verbose)
            sotl_log(INFO, "Dumping positions to file \"dump.conf\"...\n");
        if (sotl_dump_positions ("dump.conf") < 0)
            sotl_log(ERROR, "Failed to dump atom positions\n");
        if (sotl_verbose)
            sotl_log(INFO, "Done\n");
    }

    if (sotl_verbose)
        sotl_log(INFO, "Finalizing...\n");

    for (unsigned d = 0; d < sotl_nb_devices; d++) {
      switch (sotl_devices[d]->compute) {
      case SOTL_COMPUTE_SEQ :
	seq_finalize (sotl_devices[d]);
	break;
#ifdef HAVE_OMP
      case SOTL_COMPUTE_OMP :
	omp_finalize (sotl_devices[d]);
	break;
#endif
      default :
	;
      }
    }

    /* Cleanup OpenCL and OpenGL resources. */
    ocl_finalize ();
#ifdef HAVE_LIBGL
    if (sotl_display)
        vbo_finalize ();
#endif

    /* Cleanup global domain. */
    domain_free (get_global_domain ());

    /* Cleanup devices and platforms. */
    for (unsigned d = 0; d < nb_ocl_devs; d++) {
        free (all_devices[d].name);
    }
    for (unsigned int p = 0; p < nb_ocl_pfs; p++) {
        free (all_platforms[p].name);
        free (all_platforms[p].vendor);
    }

    if (sotl_verbose)
        sotl_log(INFO, "Done\n");
}
