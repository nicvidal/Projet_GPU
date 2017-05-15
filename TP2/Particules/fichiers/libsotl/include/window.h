
#ifndef WINDOW_H
#define WINDOW_H

#include "default_defines.h"

void window_opengl_init (unsigned winx, unsigned winy, calc_t *min_ext, calc_t *max_ext, int full_speed);

void window_main_loop(void (*callback)(void), unsigned nb_iter);

void setZcutValues(float zcut0, float zcut1, float zcut2);

extern float zcut[3];

void resetAnimation (void);

// get display device
//
sotl_device_t *sotl_display_device (void);

#endif
