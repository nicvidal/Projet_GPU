
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#include "default_defines.h"
#include "tools.h"
#include "sotl.h"

char *MD_FILE = "conf/default.conf";

/* Molecular dynamics parameters. */
static const double md_delta_t = 1.0;

/* Lennard Jones parameters. */
static const double lj_sigma    = 0.503968201;
static const double lj_epsilon  = 0.001;
static const double lj_rcut     = 0.6047618412; /* 1.2 * lj_sigma */;

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [ options ] { config_file }\n", name);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "\t-h | --help\t\t\tDisplay this help\n");
    fprintf(stderr, "\t-v | --verbose\t\t\tVerbose mode\n");
    fprintf(stderr, "\t-l | --list-devices\t\tShow list of OpenCL devices\n");
    fprintf(stderr, "\t-r | --rotate\t\t\tActivate rotating camera\n");
    fprintf(stderr, "\t-c | --cpu\t\t\tSelect all CPU devices\n");
    fprintf(stderr, "\t-g | --gpu\t\t\tSelect all GPU devices\n");
    fprintf(stderr, "\t-a | --all\t\t\tSelect all devices\n");
    fprintf(stderr, "\t-d | --device <n>\t\tSelect device #n\n");
    fprintf(stderr, "\t-o | --output-device <n>\tSet output device\n");
    fprintf(stderr, "\t-f | --file-dump\t\tDump atom positions to file\n");
    fprintf(stderr, "\t-s | --seq <n>\t\tRun sequential version over device #n\n");
    fprintf(stderr, "\t-O | --omp <n>\t\tRun openmp version over device #n\n");
    fprintf(stderr, "\t-R | --random-atoms\t\tRandomize atoms\n");
    fprintf(stderr, "\t-i | --nb_iter <n>\t\tNumber of iterations\n");
    fprintf(stderr, "\t-n | --natoms <n>\t\tNumber of atoms\n");
}

int main(int argc, char *argv[])
{
    int arg_count = argc;
    char **arg_tab = argv;

    calc_t domain_min[3], domain_max[3];
    int nb_iter_to_ignore = 0;
    long nb_iter = 0;
    bool randomize_atoms = false;
    unsigned natoms = 0;
    int ret;

    if (sotl_init() < 0) {
        fprintf(stderr, "Failed to initialize SOTL\n");
        return 1;
    }

    while (1) {
        static struct option long_options[] = {
            {"random-atoms",    no_argument,        0, 'R'},
            {"help",            no_argument,        0, 'h'},
            {"verbose",         no_argument,        0, 'v'},
            {"list_devices",    no_argument,        0, 'l'},
            {"all",             no_argument,        0, 'a'},
            {"gpu",             no_argument,        0, 'g'},
            {"cpu",             no_argument,        0, 'c'},
            {"file-dump",       no_argument,        0, 'f'},
            {"nb-iter",         required_argument,  0, 'i'},
            {"natoms",          required_argument,  0, 'n'},
            {"device",          required_argument,  0, 'd'},
            {"seq",             required_argument,  0, 's'},
            {"output",          required_argument,  0, 'o'},
            {"omp",             required_argument,  0, 'O'},
            {0,0,0,0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;
        int c = getopt_long(argc, argv, "i:n:Rlvhagcfd:s:o:O:",
                            long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'i':
                nb_iter = strtol(optarg, NULL, 10);
                if (nb_iter <= 0) {
                    fprintf(stderr, "WARNING: Invalid value for iterations. Default to 1.\n");
                    nb_iter = 1;
                }
                break;
            case 'R':
                randomize_atoms = true;
                break;
            case 'n':
                {
                    char unit;
                    int ret;

                    ret = sscanf(optarg, "%u%[mMkK]", &natoms, &unit);
                    if (ret == 2) {
                        if (unit == 'k' || unit == 'K') {
                            natoms *= 1000;
                        } else if (unit == 'm' || unit == 'M') {
                            natoms *= 1000000;
                        }
                    }
                }
                break;
            case 'h':
                usage(argv[0]);
                exit(0);
                break;
            case 'v':
                sotl_enable_verbose();
                break;
            case 'l':
                sotl_list_devices();
		exit(0);
                break;
            case 'a':
                sotl_add_ocl_device_by_type(SOTL_ALL);
                break;
            case 'g':
                sotl_add_ocl_device_by_type(SOTL_GPU);
                break;
            case 'c':
                sotl_add_ocl_device_by_type(SOTL_CPU);
                break;
            case 'f':
                sotl_enable_dump();
                break;
            case 'd':
                sotl_add_ocl_device_by_id(atoi(optarg));
                break;
            case 's':
                sotl_add_seq_device_by_id(atoi(optarg));
                break;
            case 'o':
                sotl_set_output_device(atoi(optarg));
                break;
            case 'O':
                sotl_add_omp_device_by_id(atoi(optarg));
                break;
            case '?':
                /* getopt_long already printed an error message. */
                break;
            default:
                usage(argv[0]);
                break;
        }
    }

    // Check if config file was specified
    //
    if (optind < argc)
        MD_FILE = argv[optind++];

    FILE *fp = NULL;
    bool read_speed = false;
    unsigned xytiles, ztiles;

    if (natoms != 0) {

      xytiles = (int)pow((float)natoms / 4.0, 1 / 3.0);

      ztiles = ((natoms + 4 * xytiles * xytiles - 1) / (4 * xytiles * xytiles));

      if (randomize_atoms) {
	xytiles *= 2;
      }

      for (int l = 0; l < 3; ++l) {
	domain_min[l] = 0.0;
	domain_max[l] = domain_min[l] +
	  ((randomize_atoms || l < 2) ? xytiles : ztiles) * LATTICE_TILE - LATTICE_TILE * 0.5;
      }

    } else {
      fp = fopen (MD_FILE, "r");
      if (fp == NULL) {
	perror("fopen");
	exit(1);
      }

      psotl_read_file_header(fp, &natoms, domain_min, domain_max, &read_speed); 
    }

    double xrange[2], yrange[2], zrange[2];

    xrange[0] = domain_min[0];
    xrange[1] = domain_max[0];
    yrange[0] = domain_min[1];
    yrange[1] = domain_max[1];
    zrange[0] = domain_min[2];
    zrange[1] = domain_max[2];
    ret = sotl_domain_init(xrange, yrange, zrange, NULL, natoms);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize the global domain : '%s'.\n",
                strerror(-ret));
        return 1;
    }

    if (fp != NULL)
      psotl_read_file_body (fp, natoms, read_speed);
    else {
      if (randomize_atoms) {
        for (int i = 0; i < natoms; ++i)
          sotl_add_random_atom();
      } else {
	psotl_lattice_atoms (natoms, xytiles, ztiles);
      }
    }

    /* Set Molecular Dynamics parameters. */
    sotl_set_parameter(MD_DELTA_T,  (void *)&md_delta_t);

    /* Set Lennard Jones parameters. */
    sotl_set_parameter(LJ_SIGMA,    (void *)&lj_sigma);
    sotl_set_parameter(LJ_EPSILON,  (void *)&lj_epsilon);
    sotl_set_parameter(LJ_RCUT,     (void *)&lj_rcut);

    ret = sotl_runtime_init();
    if (ret < 0) {
        fprintf(stderr, "Failed to distribute atoms among selected devices = '%s'.\n",
                strerror(-ret));
        return 1;
    }

    sotl_main_loop (nb_iter);

    sotl_finalize ();

    return 0;
}
