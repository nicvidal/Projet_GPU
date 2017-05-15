
#include <SDL_image.h>

#include "constants.h"
#include "graphics.h"
#include "compute.h"
#include "spiral.h"
#include "error.h"
#include "debug.h"

static SDL_Window *win = NULL;
static SDL_Renderer *ren = NULL;
static SDL_Surface *surface = NULL;

static char *progname = NULL;
static char *pngfile = NULL;
static int display = 1;

Uint32 *image = NULL;
unsigned DIM = 0;

unsigned do_first_touch = 0;

unsigned twists = 32;

//#define USE_PREFERRED_FORMAT

void usage (int val)
{
  fprintf (stderr, "Usage: %s [option]\n", progname);
  fprintf (stderr, "option can be:\n");
  fprintf (stderr, "\t-nvs\t| --no-vsync\t\t: disable vertical screen sync\n");
  fprintf (stderr, "\t-n\t| --no-display\t\t: avoid graphical display overhead\n");
  fprintf (stderr, "\t-l\t| --load-image <file>\t: use PNG image <file>\n");
  fprintf (stderr, "\t-s\t| --size <DIM>\t\t: use image of size DIM x DIM\n");
  fprintf (stderr, "\t-t\t| --twists <N>\t\t: use spirals with N twists \n");
  fprintf (stderr, "\t-d\t| --debug-flags <flags>\t: enable debug messages\n");
  fprintf (stderr, "\t-v\t| --version <x>\t\t: select version <x> of algorithm\n");
  fprintf (stderr, "\t-ft\t| --first-touch\t\t: touch memory on different cores\n");
  fprintf (stderr, "\t-h\t| --help\t\t: display help\n");

  exit (val);
}

void graphics_display_line (unsigned i)
{
  for (unsigned c = 0; c < DIM; c++)
    PRINT_DEBUG ('g', "pixel(%d,%d) = %08x\n", c, i, img(i,c));
}

static void graphics_create_surface (unsigned dim)
{
  Uint32 rmask, gmask, bmask, amask;

  amask = 0xff000000;
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;

  DIM = dim;
  image = malloc (dim * dim * sizeof (Uint32));

  if (do_first_touch) {
    void_func_t ft = first_touch [version];
    if (ft != NULL) {
      printf ("(avec first touch)\n");
      ft ();
    } else
      printf ("(pas de first touch disponible pour cette version)\n");
  } else
    printf ("(sans first touch)\n");

  surface = SDL_CreateRGBSurfaceFrom (image, dim, dim, 32,
                                      dim * sizeof (Uint32),
                                      rmask, gmask, bmask, amask);
  if (surface == NULL)
    exit_with_error ("SDL_CreateRGBSurfaceFrom () failed: %s", SDL_GetError ());
}

static void graphics_load_surface (char *filename)
{
#ifdef USE_PREFERRED_FORMAT
  // Chargement de l'image
  surface = IMG_Load (filename);
  if (surface == NULL)
    exit_with_error ("IMG_Load: <%s>\n", filename);
#else
  SDL_Surface *old;
  unsigned size;

  // Chargement de l'image
  old = IMG_Load (filename);
  if (old == NULL)
    exit_with_error ("IMG_Load: <%s>\n", filename);

  size = MIN (old->w, old->h);
  if (DIM)
    size = MIN (DIM, size);
  
  graphics_create_surface (size);

  // copie de old vers surface
  {
    SDL_Rect src;

    src.x = 0;
    src.y = 0;
    src.w = size;
    src.h = size;

    SDL_BlitSurface (old, /* src */
		     &src,
		     surface, /* dest */
		     NULL);
  }

  SDL_FreeSurface (old);
#endif
}

void graphics_image_init (void)
{
  Uint8 r, b, g, a;
  Uint32 *p;
  SDL_PixelFormat * fmt = surface->format;
  int bpp = surface->format->BytesPerPixel;
  unsigned nbits = 0;
  unsigned rb, bb, gb;
  unsigned r_lsb, g_lsb, b_lsb;
  unsigned r_shift, g_shift, b_shift;
  Uint8 r_mask, g_mask, b_mask;
  Uint8 red = 0, blue = 0, green = 0;
  
  PRINT_DEBUG ('g', "Native image size: %dx%d\n", surface->w, surface->h);

  // Calcul du nombre de bits nécessaires pour mémoriser une valeur
  // différente pour chaque pixel de l'image
  for (int i = DIM-1; i; i >>= 1)
    nbits++; // log2(DIM-1)
  nbits = nbits * 2;

  if (nbits > 24)
    exit_with_error ("DIM of %d is too large (suggested max: 4096)\n", DIM);
  
  gb = nbits / 3;
  bb = gb;
  rb = nbits - 2 * bb;

  r_shift = 8 - rb; r_lsb = (1 << r_shift) - 1; 
  g_shift = 8 - gb; g_lsb = (1 << g_shift) - 1;
  b_shift = 8 - bb; b_lsb = (1 << b_shift) - 1;

  r_mask = (1 << rb) - 1;
  g_mask = (1 << gb) - 1;
  b_mask = (1 << bb) - 1;

  PRINT_DEBUG ('g', "nbits : %d (r: %d, g: %d, b: %d)\n", nbits, rb, gb, bb);
  PRINT_DEBUG ('g', "r_shift: %d, r_lst: %d, r_mask: %d\n", r_shift, r_lsb, r_mask);
  PRINT_DEBUG ('g', "g_shift: %d, g_lst: %d, g_mask: %d\n", g_shift, g_lsb, g_mask);
  PRINT_DEBUG ('g', "b_shift: %d, b_lst: %d, b_mask: %d\n", b_shift, b_lsb, b_mask);
  
  for (unsigned y = 0; y < surface->h; y++) {
    for (unsigned x = 0; x < surface->w; x++) {
      p = (Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch + x * bpp);

      if (x < DIM && y < DIM) {
	SDL_GetRGBA (*p, fmt, &r, &g, &b, &a);

	if (a == 0 || x == 0 || x == DIM-1 || y == 0 || y == DIM-1)
	  *p = 0;
	else
#ifdef USE_PREFERRED_FORMAT
	  *p = SDL_MapRGBA (fmt,
			    (red   << r_shift) | r_lsb /* r */,
			    (green << g_shift) | g_lsb /* g */,
			    (blue  << b_shift) | b_lsb /* b */,
			    255 /* alpha */) | 0xff000000;
#else
	  *p = SDL_MapRGBA (fmt,
			    (red   << r_shift) | r_lsb /* r */,
			    (green << g_shift) | g_lsb /* g */,
			    (blue  << b_shift) | b_lsb /* b */,
			    255 /* alpha */);
#endif
	red = (red + 1) & r_mask;
	if (red == 0) {
	  green = (green + 1) & g_mask;
	  if (green == 0)
	    blue = (blue + 1) & b_mask;
	}
      } else
	*p = 0;
    }
  }

  /*
  if (debug_enabled ('g'))
    graphics_display_line (DIM-1);
  */
}

void graphics_init (int *argc, char *argv[])
{
  Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

  progname = argv[0];

  // Filter args
  //
  argv++; (*argc)--;
  while (*argc > 0) {
    if (!strcmp (*argv, "--no-vsync") || !strcmp (*argv, "-nvs")) {      
      render_flags &= ~SDL_RENDERER_PRESENTVSYNC;
    } else if (!strcmp (*argv, "--no-display") || !strcmp (*argv, "-n")) {
      display = 0;
    } else if (!strcmp (*argv, "--first-touch") || !strcmp (*argv, "-ft")) {
      do_first_touch = 1;
    } else if(!strcmp (*argv, "--help") || !strcmp (*argv, "-h")) {
      usage (0);
    } else if (!strcmp (*argv, "--load-image") || !strcmp (*argv, "-li") || !strcmp (*argv, "-l")) {
      if (*argc == 1) {
	fprintf (stderr, "Error: filename missing\n");
	usage (1);
      }
      (*argc)--; argv++;
      pngfile = *argv;
    } else if (!strcmp (*argv, "--size") || !strcmp (*argv, "-s")) {
      if (*argc == 1) {
	fprintf (stderr, "Error: DIM missing\n");
	usage (1);
      }
      (*argc)--; argv++;
      DIM = atoi(*argv);
    } else if (!strcmp (*argv, "--version") || !strcmp (*argv, "-v")) {
      if (*argc == 1) {
	fprintf (stderr, "Error: version number missing\n");
	usage (1);
      }
      (*argc)--; argv++;
      version = atoi(*argv);
    } else if (!strcmp (*argv, "--twists") || !strcmp (*argv, "-t")) {
      if (*argc == 1) {
	fprintf (stderr, "Error: N missing\n");
	usage (1);
      }
      (*argc)--; argv++;
      twists = atoi(*argv);
    } else if (!strcmp (*argv, "--debug-flags") || !strcmp (*argv, "-d")) {
      if (*argc == 1) {
	fprintf (stderr, "Error: flag list missing\n");
	usage (1);
      }
      (*argc)--; argv++;
      debug_flags = *argv;
    } else
      break;
    (*argc)--; argv++;
  }

  // Initialisation de SDL
  if (SDL_Init (SDL_INIT_VIDEO) != 0)
    exit_with_error ("SDL_Init");

  if (display) {

    // Création de la fenêtre sur l'écran
    win = SDL_CreateWindow ("Hello World!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			    WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
    if (win == NULL)
      exit_with_error ("SDL_CreateWindow");

    // Initialisation du moteur de rendu (aka renderer)
    ren = SDL_CreateRenderer (win, -1, render_flags);
    if (ren == NULL)
      exit_with_error ("SDL_CreateRenderer");
  }

  if (prelude [version] != NULL)
    prelude [version] ();
  
  printf ("Version utilisée : %s ", version_name [version]);

  if (pngfile == NULL) {
    unsigned size = DIM ? DIM : DEFAULT_DIM;
    graphics_create_surface (size);
    spiral_regular (1, size-2, 1, size-2, 2, twists);
  } else
    graphics_load_surface (pngfile);

  graphics_image_init ();

  PRINT_DEBUG ('g', "DIM = %d\n", DIM);  
}

void graphics_render_image (void)
{
  SDL_Rect src, dst;
  SDL_Texture *texture = NULL;

  SDL_RenderClear (ren);
  
  // Création d'une texture à partir de la surface
  texture = SDL_CreateTextureFromSurface(ren, surface);

  src.x = 0;
  src.y = 0;
  src.w = DIM;
  src.h = DIM;

  // On redimensionne l'image pour qu'elle occupe toute la fenêtre
  dst.x = 0;
  dst.y = 0;
  dst.w = WIN_WIDTH;
  dst.h = WIN_HEIGHT;

  SDL_RenderCopy (ren, texture, &src, &dst);

  SDL_DestroyTexture (texture);
}

void graphics_refresh (void)
{
  // On efface la scène dans le moteur de rendu (inutile !)
  //SDL_RenderClear (ren);

  // On réaffiche l'image
  graphics_render_image ();
  
  // Met à jour l'affichage sur écran
  SDL_RenderPresent (ren);
}

void graphics_clean (void)
{
  if (postlude [version] != NULL)
    postlude [version] ();
  
  if (display) {
    
    if (ren != NULL)
      SDL_DestroyRenderer (ren);
    else
      return;

    if (win != NULL)
      SDL_DestroyWindow (win);
    else
      return;
  }

  if (image != NULL)
    free (image);

  if (surface != NULL)
      SDL_FreeSurface (surface);

  IMG_Quit ();
  SDL_Quit ();
}

int graphics_display_enabled (void)
{
  return display;
}
