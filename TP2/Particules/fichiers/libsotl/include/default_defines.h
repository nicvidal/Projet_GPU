
#ifndef DEFAULT_DEFINES_H
#define DEFAULT_DEFINES_H

#include "sotl_internal.h"

#define TILE_SIZE            64
#define TILE_SIZE_BOX_OFFSET TILE_SIZE
#define SCAN_WG_SIZE         256 

// Compute forces and update positions within the same kernel
#define FORCE_N_UPDATE

#define TORUS

#define SUBCELL 1

//#define SLIDE 1
//#define TILE_CACHE

#define PROFILING

#define SHOW_GRID

//#define XEON_VECTORIZATION

// In _SPHERE_MODE_, atoms are displayed as a mesh of (many) triangles
// and look pretty nice. However, this mode incurs a high memory
// consumption on the GPU.
//
// If _SPHERE_MODE_ is undefined, then the display falls back to an
// shader-based point-based rendering.
//

//#define _SPHERE_MODE_

#define DISPLAY_XSIZE 1024
#define DISPLAY_YSIZE 768

#define OPENCL_BUILD_OPTIONS "-cl-mad-enable -cl-fast-relaxed-math "

/* Lennard Jones default parameters. */
#define LJ_SIGMA_DEFAULT_VALUE      0.5039684201
#define LJ_EPSILON_DEFAULT_VALUE    0.001
#define LJ_RCUT_DEFAULT_VALUE       (1.2 * LJ_SIGMA_DEFAULT_VALUE)

/* Lennard Jones user parameters. */
#define LENNARD_SIGMA       (*(double *)sotl_get_parameter(LJ_SIGMA))
#define LENNARD_EPSILON     (*(double *)sotl_get_parameter(LJ_EPSILON))
#define LENNARD_CUTOFF      (*(double *)sotl_get_parameter(LJ_RCUT))

#define LENNARD_SQUARED_CUTOFF (LENNARD_CUTOFF * LENNARD_CUTOFF)

////////////////////////////////////

#if defined(FORCE_N_UPDATE) && defined(SLIDE)
#error "Using SLIDE is not supported in FORCE_N_UPDATE mode"
#endif

#if defined(_SPHERE_MODE_) && USE_DOUBLE == 1
#error "Using USE_DOUBLE is not supported in SPHERE_MODE"
#endif

#define ALRND(a,n) (((size_t)(n)+(a)-1)&(~(size_t)((a)-1)))

#define ALIGN      (TILE_SIZE > 16 ? TILE_SIZE : 16)
#define ROUND(n)   ALRND(ALIGN,(n))

#define ATOM_RADIUS 0.2

#define LATTICE_TILE (ATOM_RADIUS * 4.0)

#define BOX_SIZE (LENNARD_CUTOFF / SUBCELL)
#define BOX_SIZE_INV (1.0 / BOX_SIZE)


#if USE_DOUBLE == 0
#define calc_t float
#define coord_t float
#else
#define calc_t double
#endif

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

#define check(err, ...)					\
    do {							\
        if(err != CL_SUCCESS) {				\
            char strtmp[1024]; \
            sprintf(strtmp, __VA_ARGS__);		\
            sotl_log(CRITICAL, "(%d) %s\n", err, strtmp);	\
        }							\
    } while(0)

#endif
