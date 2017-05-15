#include "atom.h"
#include "domain.h"
#include "sotl.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static sotl_domain_t global_domain;

void domain_init(sotl_domain_t *dom, const calc_t x_min, const calc_t y_min,
                 const calc_t z_min, const calc_t x_max, const calc_t y_max,
                 const calc_t z_max)
{
    /* Set initial values. */
    dom->min_ext[0] = x_min;
    dom->min_ext[1] = y_min;
    dom->min_ext[2] = z_min;
    dom->max_ext[0] = x_max;
    dom->max_ext[1] = y_max;
    dom->max_ext[2] = z_max;
    dom->total_boxes = 1;

    /* Init sub domains. */
    dom->nb_subdomains = 0;
    dom->subdomains    = NULL;

    /* XXX: Attach the global atom_set to this global domain. */
    dom->atom_set = get_global_atom_set();

    for (int i = 0; i < 3; i++) {
        /* Increase min and max pos according to the radius of atoms. */
        dom->min_ext[i] -= ATOM_RADIUS;
        dom->max_ext[i] += ATOM_RADIUS;

        /* Compute the number of boxes for this axis. */
        dom->boxes[i] = ceil((dom->max_ext[i] - dom->min_ext[i]) / BOX_SIZE);
        dom->max_ext[i] = dom->min_ext[i] + dom->boxes[i] * BOX_SIZE;

        /* Compute min and max border pos. */
        dom->min_border[i] = dom->min_ext[i] - BOX_SIZE * SUBCELL;
        dom->max_border[i] = dom->max_ext[i] + BOX_SIZE * SUBCELL;
        dom->boxes[i] += 2 * SUBCELL;

        /* Compute total number of boxes. */
        dom->total_boxes *= dom->boxes[i];
    }
}

void domain_free(sotl_domain_t *dom)
{
    /* Free sub domains. */
    for (unsigned i = 0; i < dom->nb_subdomains; i++)
        free(dom->subdomains[i]);
    free(dom->subdomains);
}

static unsigned *count_atoms_per_z_planes(const sotl_domain_t *dom)
{
    const int shift_z = dom->boxes[0] * dom->boxes[1];
    const int shift_y = dom->boxes[0];
    unsigned *z_planes = NULL;
    int *boxes = NULL;

    z_planes = calloc(dom->boxes[2], sizeof(unsigned));
    if (!z_planes)
        return NULL;

    boxes = atom_set_box_count(dom, dom->atom_set);
    if (!boxes) {
        free(z_planes);
        return NULL;
    }

    for (unsigned z = 0; z < dom->boxes[2]; z++) {
        for (unsigned y = 0; y < dom->boxes[1]; y++) {
            for (unsigned x = 0; x < dom->boxes[0]; x++) {
                unsigned idx = z * shift_z + y * shift_y + x;
                z_planes[z] += boxes[idx];
            }
        }
    }

    free(boxes);
    return z_planes;
}

static void find_zcut_values(const sotl_domain_t *dom, const unsigned *z_planes,
                             const unsigned n, int *zcuts, int *natoms)
{
    unsigned natoms_per_domains = dom->atom_set->natoms / n;
    unsigned natoms_acc = 0;
    int nzcuts = n;

    for (unsigned z = 0; z < dom->boxes[2]; z++) {
        natoms_acc += z_planes[z];

        if (natoms_acc < natoms_per_domains) {
            /* Do not split while we do not have enough atoms. */
            continue;
        }

        /* Store Z boundary and the number of atoms of this Z plan. */
        zcuts[n - nzcuts]  = z + 1;
        natoms[n - nzcuts] = natoms_acc;

        /* Find out the next domain if needed. */
        natoms_acc = 0;
        nzcuts--;

        if (nzcuts < 2) {
            /* No more domain needed. */
            break;
        }
    }
}

void domain_split(sotl_domain_t *dom, const unsigned n)
{
    unsigned *z_planes = NULL;
    int zcuts[n], natoms[n];

    /* Allocate and initialize the array of sub domains. */
    dom->nb_subdomains = n;
    dom->subdomains    = xmalloc(sizeof(*dom) * dom->nb_subdomains);
    for (unsigned int i = 0; i < dom->nb_subdomains; i++) {
        dom->subdomains[i] = xmalloc(sizeof(*dom));
        memcpy(dom->subdomains[i], dom, sizeof(*dom));
        dom->subdomains[i]->nb_subdomains = 0;
        dom->subdomains[i]->subdomains = NULL;
    }

    if (n == 1) {
        /* Do no split the domain. */
        return;
    }

    /* Count number atoms per Z planes. */
    z_planes = count_atoms_per_z_planes(dom);
    for (unsigned z = 0; z < dom->boxes[2]; z++)
        fprintf(stderr, "z_planes[%d] = %d\n", z, z_planes[z]);

    /* Find z-axis boundaries. */
    find_zcut_values(dom, z_planes, n, zcuts, natoms);
    for (unsigned i = 0; i < n - 1; i++)
        sotl_log(DEBUG, "zcuts[%d] = %d, natoms[%d] = %d\n", i, zcuts[i], i, natoms[i]);

    /* Split the domain along z-axis. */
    for (unsigned i = 0; i < dom->nb_subdomains; i++) {
        sotl_domain_t *subdom = dom->subdomains[i];
        int zcut;

        /* Find corresponding zcut for this sub domain. */
        if (i == 0) {
            zcut = zcuts[i];
        } else if (i < n - 1) {
            zcut = zcuts[i] - zcuts[i - 1];
        } else {
            zcut = dom->boxes[2] - zcuts[i - 1];
        }

        /* Set the number of boxes in z-axis. */
        subdom->boxes[2] = zcut + 1; /* +1 for "shared" border between domains. */
        if (i > 0 && i < n - 1) {
            /* +1 for intermediate domains. */
            subdom->boxes[2]++;
        }

        /* Update min and max positions of sub domains. */
        if (i > 0) {
            subdom->min_ext[2]    = dom->subdomains[i - 1]->max_ext[2];
            subdom->min_border[2] = subdom->min_ext[2] - SUBCELL * BOX_SIZE;
        }
        subdom->max_ext[2]    = subdom->min_ext[2] + (subdom->boxes[2] - 2) * BOX_SIZE;
        subdom->max_border[2] = subdom->max_ext[2] + SUBCELL * BOX_SIZE;

        /* Update the total number of boxes. */
        subdom->total_boxes = subdom->boxes[0] * subdom->boxes[1] * subdom->boxes[2];

        subdom->atom_set = xmalloc(sizeof(sotl_atom_set_t));
        sotl_atom_set_t *prev_set = NULL;
        unsigned natoms_offset;
        int natom;
        if (i < n - 1) {
            natom = natoms[i];
        } else {
            unsigned acc = 0;
            for (unsigned j = 0; j < dom->nb_subdomains - 1; j++)
                acc += natoms[j];
            natom = dom->atom_set->natoms - acc;
        }

        /* Atom set borders (left/right). */
        if (i == 0) {
            subdom->atom_set->nghosts_min = 0;
            subdom->atom_set->nghosts_max = z_planes[zcuts[i]];
        } else if (i < n - 1) {
            subdom->atom_set->nghosts_min = z_planes[zcuts[i - 1] - 1];
            subdom->atom_set->nghosts_max = z_planes[zcuts[i]];
        } else {
            subdom->atom_set->nghosts_min = z_planes[zcuts[i - 1] - 1];
            subdom->atom_set->nghosts_max = z_planes[dom->boxes[2] - 1];
        }

        /* Atom set */
        subdom->atom_set->natoms  = natom;
        subdom->atom_set->current = natom;
        subdom->atom_set->offset  = ROUND(natom);
        subdom->atom_set->offset_ghosts = dom->atom_set->offset_ghosts;

        if (i == 0) {
            /* The first sub atom set points to the global atom set. */
            prev_set      = dom->atom_set;
            natoms_offset = 0;
        } else {
            /* Others sub sets point to the previous one. */
            prev_set      = dom->subdomains[i - 1]->atom_set;
            natoms_offset = prev_set->natoms;
        }

        /* Initialize atom positions and speeds. */
        subdom->atom_set->pos.x    = prev_set->pos.x    + natoms_offset;
        subdom->atom_set->pos.y    = prev_set->pos.y    + natoms_offset;
        subdom->atom_set->pos.z    = prev_set->pos.z    + natoms_offset;
        subdom->atom_set->speed.dx = prev_set->speed.dx + natoms_offset;
        subdom->atom_set->speed.dy = prev_set->speed.dy + natoms_offset;
        subdom->atom_set->speed.dz = prev_set->speed.dz + natoms_offset;
    }

    free(z_planes);

#if 0 /* XXX: For debugging purposes. */
    fprintf(stderr, "------\n");
    domain_print(get_global_domain());
    for (int i = 0; i < dom->nb_subdomains; i++) {
        fprintf(stderr, "------\n");
        domain_print(dom->subdomains[i]);
        atom_set_print(dom->subdomains[i]->atom_set);
    }
    unsigned int total_boxes = 0, total_atoms = 0, total_offset = 0;
    for (unsigned i = 0; i < dom->nb_subdomains; i++) {
        total_boxes += dom->subdomains[i]->boxes[2];
        total_atoms += dom->subdomains[i]->atom_set->natoms;
        total_offset += dom->subdomains[i]->atom_set->offset;
    }
    fprintf(stderr, "total_atoms = %d, total_offset = %d\n",
            total_atoms, total_offset);
    fprintf(stderr, "----- %d\n", dom->atom_set->natoms);
    assert(total_atoms == dom->atom_set->natoms);
    assert(total_boxes != dom->total_boxes);
#endif
}

void domain_print(const sotl_domain_t *dom)
{
    for (int i = 0; i < 3; i++) {
        sotl_log(DEBUG, "%c: [%f [%f, %f] %f]\n", "xyz"[i],
                 dom->min_border[i], dom->min_ext[i], dom->max_ext[i],
                 dom->max_border[i]);

    }
    sotl_log(DEBUG, "%d x %d x %d = %d boxes\n", dom->boxes[0], dom->boxes[1],
             dom->boxes[2], dom->total_boxes);
}

sotl_domain_t *get_global_domain()
{
    return &global_domain;
}
