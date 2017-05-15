#ifndef __ATOM_SET_H
#define __ATOM_SET_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "default_defines.h"
#include "domain.h"
#include "global_definitions.h"
#include "util.h"

/**
 * A structure to represent atom positions
 * (looks like 3 arrays, but allocated once contiguously)
 */
typedef struct {
    calc_t *x;
    calc_t *y;
    calc_t *z;
} sotl_atom_pos_t;

/**
 * A structure to represent atom speeds
 * (looks like 3 arrays, but allocated once contiguously)
 */
typedef struct {
    calc_t *dx;
    calc_t *dy;
    calc_t *dz;
} sotl_atom_speed_t;

/**
 * A structure to represent atom sets.
 */
typedef struct atom_set {
    sotl_atom_pos_t pos;        /**< positions in x, y, z */
    sotl_atom_speed_t speed;    /**< speeds in dx, dy, dz */
    unsigned natoms;            /**< total number of atoms */
    unsigned current;           /**< current number of atoms (eg. during filling phase) */
    unsigned offset;            /**< maximum capacity of atoms */

    /* Only used in multi devices. */
    unsigned nghosts_min;       /**< number of ghosts on left border in Z */
    unsigned nghosts_max;       /**< number of ghosts on right border in Z */
    unsigned offset_ghosts;     /**< maximum capacity of ghosts */
} sotl_atom_set_t;

/**
 * Allocate and initialize an atom set.
 *
 * @param set The atom set to initialize.
 * @param natoms The total number of atoms.
 * @param maxatoms The maximum capacity (must be equal or greater than natoms).
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_OUT_OF_MEMORY if there is a failure to allocate ressources
 *         - SOTL_INVALID_VALUE if maxatoms if less than natoms
 */
int atom_set_init(sotl_atom_set_t *set, const unsigned long natoms,
                  const unsigned long maxatoms);

/**
 * Print an atom set.
 */
void atom_set_print(const sotl_atom_set_t *set);

/**
 * Add a new atom to the atom set.
 *
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_INVALID_BUFFER_SIZE if the maximum capcity is reached
 */
int atom_set_add(sotl_atom_set_t *set, const calc_t x, const calc_t y,
                 const calc_t z, const calc_t dx, const calc_t dy,
                 const calc_t dz);

/**
 * Get address of the global atom set.
 */
sotl_atom_set_t *get_global_atom_set();

/**
 * Get maximum number of atoms in the atom set.
 */
size_t atom_set_offset(const sotl_atom_set_t *set);

/**
 * Get size in bytes of the atom set.
 */
size_t atom_set_size(sotl_atom_set_t *set);

/**
 * Get size in bytes of one border (only in multi devices).
 */
size_t atom_set_border_size(const sotl_atom_set_t *set);

/**
 * Return begin offset of the atom set.
 */
size_t atom_set_begin(const sotl_atom_set_t *set);

/**
 * Return end offset of the atom set.
 */
size_t atom_set_end(const sotl_atom_set_t *set);

/**
 * Free memory allocated by the atom set.
 */
void atom_set_free(sotl_atom_set_t *set);

/**
 * Sort atoms along z-axis using a heap sort algorithm.
 */
void atom_set_sort(sotl_atom_set_t *set);

void atom_build (int natoms, sotl_atom_pos_t* pos_vec);

/**
 * Get the corresponding box of an atom.
 */
int atom_get_num_box(const sotl_domain_t *dom, const calc_t x, const calc_t y,
                     const calc_t z, const calc_t rrc);

/**
 * Count the number of atoms per boxes.
 * This functions returns an allocated buffer.
 */
int *atom_set_box_count(const sotl_domain_t *dom, const sotl_atom_set_t *set);

#endif /* __ATOM_SET_H */
