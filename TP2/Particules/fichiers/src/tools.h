/* TOOLS_H */
#ifndef TOOLS_H
#define TOOLS_H

#include <stdbool.h>

#include "sotl.h"

int psotl_read_file_header (FILE *fd, unsigned *natoms, 
			   calc_t *min_ext, calc_t *max_ext, bool *read_speed);

int psotl_read_file_body (FILE *fd, unsigned natoms_to_read, bool read_speed);

int psotl_lattice_atoms(int natoms_to_gen, unsigned xytiles, unsigned ztiles);

#endif
