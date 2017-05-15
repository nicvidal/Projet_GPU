
#include "compute.h"
#include "graphics.h"
#include "debug.h"
#include "ocl.h"

#include <stdbool.h>

unsigned version = 0;

void first_touch_omp (void);

unsigned compute_v0 (unsigned nb_iter);
unsigned compute_v1 (unsigned nb_iter);
unsigned compute_v2 (unsigned nb_iter);

void_func_t first_touch [] = {
  NULL,
  first_touch_omp,
  NULL,
};

int_func_t compute [] = {
  compute_v0,
  compute_v1,
  compute_v2,
};

char *version_name [] = {
  "Séquentielle",
  "OpenMP",
  "OpenCL",
};

unsigned opencl_used [] = {
  0,
  0,
  1,
};

///////////////////////////// Version séquentielle simple


// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
unsigned compute_v0 (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // Transposée
    for (int i = 0; i < DIM; i++)
      for (int j = 0; j < DIM; j++)
	next_img (i, j) = cur_img (j, i);

    // return it;
    swap_images ();
  }

  return 0; // on n'a pas convergé cette fois-ci
}

///////////////////////////// Version OpenMP avec omp for

void first_touch_omp ()
{
  int i,j ;

#pragma omp parallel for
  for(i=0; i<DIM ; i++) {
    for(j=0; j < DIM ; j += 512)
      next_img (i, j) = cur_img (i, j) = 0 ;
  }
}


unsigned compute_v1 (unsigned nb_iter)
{
  return 0;
}  


///////////////////////////// Version OpenCL

// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
unsigned compute_v2 (unsigned nb_iter)
{
  return ocl_compute (nb_iter);
}
