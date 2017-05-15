#ifndef __DOMAIN_H
#define __DOMAIN_H

#include "default_defines.h"

/**
 * A structure to represent domains.
 */
typedef struct domain {
    calc_t min_border[3], max_border[3];    /**< min and max border pos in x, y, z */
    calc_t min_ext[3], max_ext[3];          /**< min and max pos in x, y, z */
    unsigned boxes[3];                      /**< number of boxes in x, y, z */
    unsigned total_boxes;                   /**< total number of boxes */
    unsigned nb_subdomains;                 /**< number of sub domains. */
    struct domain **subdomains;             /**< array of sub domainbs. */
    struct atom_set *atom_set;              /**< atom set. */
} sotl_domain_t;

/**
 * Initialize a domain.
 */
void domain_init(sotl_domain_t *dom, const calc_t x_min, const calc_t y_min,
                 const calc_t z_min, const calc_t x_max, const calc_t y_max,
                 const calc_t z_max);

/**
 * Split a domain into n sub domains.
 */
void domain_split(sotl_domain_t *dom, const unsigned n);

/**
 * Free memory allocated by a domain.
 */
void domain_free(sotl_domain_t *dom);

/**
 * Print a domain.
 *
 * @param dom       The domain to print.
 */
void domain_print(const sotl_domain_t *dom);

/**
 * Get the global domain.
 */
sotl_domain_t *get_global_domain();

#endif /* __DOMAIN_H */
