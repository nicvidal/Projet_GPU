#ifndef SOTL_H
#define SOTL_H

#ifdef __cplusplus
extern "C" {
#endif

#if USE_DOUBLE == 0
#define calc_t float
#else
#define calc_t double
#endif

enum {
    DEBUG = 1,
    INFO,
    PERF,
    WARNING,
    ERROR,
    CRITICAL,
} SOTL_LOG_TYPE;

enum {
    SOTL_SUCCESS = 0,
    SOTL_OUT_OF_MEMORY = -1,

    SOTL_INVALID_VALUE = -30,
    SOTL_INVALID_BUFFER_SIZE = -31,
    SOTL_INVALID_DEVICE = -32,
    SOTL_INVALID_DEVICE_TYPE = -33,
    SOTL_DEVICE_NOT_FOUND = -34,

    SOTL_INVALID_PARAMETER = -35,

    SOTL_NOT_IMPLEMENTED = -99,
} SOTL_ERRORS;

typedef enum {
    SOTL_CPU = 0,
    SOTL_GPU,
    SOTL_OTHER,
    SOTL_ALL,
} sotl_device_type;

typedef enum {
    /* Molecula Dynamics parameters. */
    MD_DELTA_T = 0,

    /* Lennard Jones parameters. */
    LJ_SIGMA,
    LJ_EPSILON,
    LJ_RCUT,
} sotl_parameters;

/**
 * Display a log message.
 */
void sotl_log(const int type, const char *fmt, ...);

/**
 * Return 1 when we use multi GPUS, 0 otherwise.
 */
int sotl_have_multi();

/**
 * Initialize SOTL.
 */
int sotl_init();

/**
 * Finalize and cleanup resources used by SOTL.
 */
void sotl_finalize();

/**
 * List all available OpenCL devices.
 */
void sotl_list_devices();

/**
 * Enable verbose mode.
 */
void sotl_enable_verbose();

/**
 * Enable dump positions.
 */
void sotl_enable_dump();

/**
 * Add an OpenCL device by type.
 *
 * @param type Device type.
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_INVALID_DEVICE_TYPE if the device type is not OpenCL valid
 *         - SOTL_DEVICE_NOT_FOUND if no devices are found
 */
int sotl_add_ocl_device_by_type(const sotl_device_type type);

/**
 * Add an OpenCL device by id.
 *
 * @param dev_id Device ID.
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_INVALID_DEVICE if the device ID is invalid
 */
int sotl_add_ocl_device_by_id(const unsigned dev_id);

/**
 * Add a sequential (CPU) device by id.
 *
 * @param dev_id Device ID.
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_INVALID_DEVICE if the device ID is invalid
 *         - SOTL_INVALID_DEVICE_TYPE if the device is not a CPU
 */
int sotl_add_seq_device_by_id(const unsigned dev_id);

/**
 * Add an OpenMP (CPU) device by id.
 *
 * @param dev_id Device ID.
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_INVALID_DEVICE if the device ID is invalid
 *         - SOTL_INVALID_DEVICE_TYPE if the device is not a CPU
 */
int sotl_add_omp_device_by_id(const unsigned dev_id);

/**
 * Define the output device for OpenGL rendering.
 *
 * @param dev_id Device ID.
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_INVALID_DEVICE if the device ID is invalid
 */
int sotl_set_output_device(const unsigned dev_id);

/**
 * Get the number of selected GPU devices.
 *
 * @return Return the number of GPUs.
 */
int sotl_get_selected_gpus();

/**
 * Get the number of select MIC devices.
 *
 * @return Return the number of MICs.
 */
int sotl_get_selected_mics();

/**
 * Initialize the domain.
 *
 * @param xrange    Range on X axis (min/max)
 * @param yrange    Range on Y axis (min/max)
 * @param zrange    Range on Z axis (min/max)
 * @param cellbox   Box size in x, y and z.
 * @param natoms    Total number of atoms by default.
 *
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_INVALID_VALUE if xrange, yrange or zrange are invalid
 */
int sotl_domain_init(const double *xrange, const double *yrange,
                     const double *zrange, const double *cellbox,
                     const unsigned natoms);
/**
 * Set a parameter.
 *
 * @param name      Parameter name
 * @param value     Parameter value
 *
 * The following parameters are currently available:
 *  - <name>        <type>
 *  - MD_DELTA_T    (double)
 *  - LJ_SIGMA      (double)
 *  - LJ_EPSILON    (double)
 *  - LJ_RCUT       (double)
 *
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_INVALID_PARAMETER if the parameter name is invalid
 */
int sotl_set_parameter(const unsigned int name, const void *value);

/**
 * Get a parameter.
 *
 * @param name      Parameter name.
 *
 * Refer to sotl_set_paramter() for the list of available parameters.
 *
 * @return Return a pointer to the value if the function is executed successfully.
 *         Otherwise, it returns NULL if the paramenter name is invalid.
 */
const void *sotl_get_parameter(const unsigned int name);

/**
 * Synchronize all atom ids, positions and speeds.
 *
 * @param ids       Pointer to the array of ids (unused, set it to NULL).
 * @param pos_x     Pointer to the array of positions in X.
 * @param pos_x     Pointer to the array of positions in Y.
 * @param pos_x     Pointer to the array of positions in Z.
 * @param spd_x     Pointer to the array of speeds in X.
 * @param spd_y     Pointer to the array of speeds in Y.
 * @param spd_z     Pointer to the array of speeds in Z.
 * @param natoms    Maximum number of atoms to synchronize.
 *
 * @return Return SOTL_SUCCESS if the function is executed successfully.
 *         Otherwise, it returns one of the following errors :
 *         - SOTL_INVALID_VALUE if the maximum number of atoms to
 *           synchronize is not equal to the total number of atoms.
 */
int sotl_sync_all_atoms(unsigned *ids, calc_t *pos_x, calc_t *pos_y,
                        calc_t *pos_z, calc_t *spd_x, calc_t *spd_y,
                        calc_t *spd_z, const unsigned natoms);

/**
 * Update position 1.
 */
int sotl_push_pos_1();

/**
 * Update position 2.
 */
int sotl_push_pos_2();

/**
 * Compute force.
 */
int sotl_compute_force();

/**
 * Reset force.
 */
int sotl_reset_force();

void sotl_add_atom (calc_t x, calc_t y, calc_t z, 
		    calc_t dx, calc_t dy, calc_t dz);

/**
 * Add an atom with random positions and speeds.
 */
void sotl_add_random_atom();

/**
 * Distribute atoms among selected devices.
 */
int sotl_runtime_init();

// Enter main loop
//
void sotl_main_loop (unsigned nb_iter);

// Dump atom positions to disk
//
int sotl_dump_positions(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
