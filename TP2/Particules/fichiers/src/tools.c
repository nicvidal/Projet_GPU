#define _XOPEN_SOURCE 600

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "sotl.h"
#include "default_defines.h"

static calc_t rand_calc_t (calc_t mn, calc_t mx);

static calc_t rand_calc_t (calc_t mn, calc_t mx)
{
    calc_t r = random () / (calc_t) RAND_MAX;
    return mn + (mx - mn) * r;
}

int psotl_read_file_header(FILE *fd, unsigned *natoms,
			   calc_t *min_ext, calc_t *max_ext, bool *read_speed)
{
  int read_speed_int; 

  fscanf(fd, "%d", natoms);

  for (int l = 0; l < 3; l++) {
#if USE_DOUBLE == 0
    fscanf (fd, "%f %f", &min_ext[l], &max_ext[l]);
#else
    fscanf (fd, "%lf %lf", &min_ext[l], &max_ext[l]);
#endif
  }

  fscanf(fd, "%d", &read_speed_int);
  *read_speed = (bool)read_speed_int;

  return 0;
}

int psotl_read_file_body(FILE *fd,
			 unsigned natoms_to_read, bool read_speed)
{
  calc_t posx, posy, posz, spdx, spdy, spdz;

  for (int i = 0; i < natoms_to_read; i++) {
#if USE_DOUBLE == 0
    fscanf(fd, "%f %f %f", &posx, &posy, &posz);
#else
    fscanf(fd, "%lf %lf %lf", &posx, &posy, &posz);
#endif
    if (read_speed) {
#if USE_DOUBLE == 0
      fscanf(fd, "%f %f %f", &spdx, &spdy, &spdz);
#else
      fscanf(fd, "%lf %lf %lf", &spdx, &spdy, &spdz);
#endif
    } else {
      // Velocity norm is between 5% and 20% of atom radius, in a
      // random direction
      calc_t speed = rand_calc_t (ATOM_RADIUS * 0.05, ATOM_RADIUS * 0.2);
      calc_t lat = rand_calc_t (-M_PI / 2, M_PI / 2);
      calc_t lon = rand_calc_t (0.0, M_PI * 2);
      spdx = cos (lon) * cos (lat) * speed;
      spdy = sin (lat) * speed;
      spdz = -sin (lon) * cos (lat) * speed;
    }

    sotl_add_atom (posx, posy, posz, spdx, spdy, spdz);
  }

  return 0;
}

static unsigned nb_atoms = 0;

void add_atom(unsigned x, unsigned y, unsigned z,
	      float xs, float ys, float zs,
	      unsigned xytiles)
{
  float xpos, ypos, zpos;

  if (x <= 2*xytiles && y <= 2*xytiles) {

    xpos = x * LATTICE_TILE / 2.0;
    ypos = y * LATTICE_TILE / 2.0;
    zpos = z * LATTICE_TILE / 2.0;

    sotl_add_atom (xpos, ypos, zpos, xs, ys, zs);

    nb_atoms++;
  }
}

int psotl_lattice_atoms(int natoms_to_gen, unsigned xytiles, unsigned ztiles)
{

  nb_atoms = 0;

  for(unsigned z = 0; z < ztiles * 2; z += 2) {
    for(unsigned y = 0; y < xytiles * 2; y += 2) {
      for(unsigned x = 0; x < xytiles * 2; x += 2) {

	// Place 4 atoms per lattice
	// 0, 0, 0
	if(nb_atoms < natoms_to_gen)
	add_atom(x, y, z,
		 0.0, 0.0, 0.0,
		 xytiles);
	// 0, 1/2, 1/2
	if(nb_atoms < natoms_to_gen)
	add_atom(x, y + 1, z + 1,
		 0.0, 0.0, 0.0,
		 xytiles);
	// 1/2, 0, 1/2
	if(nb_atoms < natoms_to_gen)
	add_atom(x + 1, y, z + 1,
		 0.0, 0.0, 0.0,
		 xytiles);
	// 1/2, 1/2, 0 
	if(nb_atoms < natoms_to_gen)
	add_atom(x + 1, y + 1, z,
		 0.0, 0.0, 0.0,
		 xytiles);
      }
    }
  }

  return 0;
}
