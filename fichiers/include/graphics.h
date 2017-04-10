
#ifndef GRAPHICS_IS_DEF
#define GRAPHICS_IS_DEF


#include <SDL.h>

#include "constants.h"

void graphics_init (int *argc, char *argv[]);
void graphics_share_texture_buffers (void);
void graphics_refresh (void);
void graphics_clean (void);
int graphics_display_enabled (void);

extern unsigned refresh_rate;
extern int max_iter;

extern unsigned DIM;

extern Uint32 *image, *alt_image;

static inline Uint32 *img_cell (Uint32 *i, int l, int c)
{
  return i + l * DIM + c;
}

#define cur_img(y,x) (*img_cell(image,(y),(x)))
#define next_img(y,x) (*img_cell(alt_image,(y),(x)))

static inline void swap_images (void)
{
  Uint32 *tmp = image;
  
  image = alt_image;
  alt_image = tmp;
}

#endif
