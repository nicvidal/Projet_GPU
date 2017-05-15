#ifndef SOTL_INTERNAL_H
#define SOTL_INTERNAL_H

#include "sotl.h"

typedef struct {
    /* Molecular dynamics parameters. */
    struct {
        double delta_t;
    } md;

    /* Lennard Jones parameters. */
    struct {
        double sigma;
        double epsilon;
        double rcut;
    } lj;
} sotl_params_t;

sotl_params_t *get_params();

#endif /* SOTL_INTERNAL_H */
