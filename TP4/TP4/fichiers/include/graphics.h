
#ifndef GRAPHICS_IS_DEF
#define GRAPHICS_IS_DEF


#include <SDL.h>

#include "constants.h"

void graphics_init (int *argc, char *argv[]);
void graphics_refresh (void);
void graphics_clean (void);
int graphics_display_enabled (void);
void graphics_display_line (unsigned i); // for debugging purpose

extern unsigned DIM;

extern Uint32 *image;

static inline Uint32 *img_cell (int l, int c)
{
  return image + l * DIM + c;
}

#define img(y,x) (*img_cell((y),(x)))


#endif
