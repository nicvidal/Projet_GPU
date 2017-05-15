
#include <SDL_image.h>
#include <SDL_opengl.h>

#include "constants.h"
#include "graphics.h"
#include "compute.h"
#include "draw.h"
#include "error.h"
#include "debug.h"
#include "ocl.h"

static SDL_Window *win = NULL;
static SDL_Renderer *ren = NULL;
static SDL_Surface *surface = NULL;
static SDL_Texture *texture = NULL;
//static SDL_Texture *alt_texture = NULL;

static char *progname = NULL;
static char *pngfile = NULL;
static int display = 1;

Uint32 *image = NULL, *alt_image = NULL;
unsigned DIM = 0;

unsigned do_first_touch = 0;

int max_iter = 0;

unsigned refresh_rate = 1;

void usage (int val)
{
  fprintf (stderr, "Usage: %s [option]\n", progname);
  fprintf (stderr, "option can be:\n");
  fprintf (stderr, "\t-nvs\t| --no-vsync\t\t: disable vertical screen sync\n");
  fprintf (stderr, "\t-n\t| --no-display\t\t: avoid graphical display overhead\n");
  fprintf (stderr, "\t-l\t| --load-image <file>\t: use PNG image <file>\n");
  fprintf (stderr, "\t-s\t| --size <DIM>\t\t: use image of size DIM x DIM\n");
  fprintf (stderr, "\t-i\t| --iterations <n>\t\t: stop after n iterations\n");
  fprintf (stderr, "\t-r\t| --refresh-rate <N>\t\t: display only 1/Nth of images\n");
  fprintf (stderr, "\t-d\t| --debug-flags <flags>\t: enable debug messages\n");
  fprintf (stderr, "\t-v\t| --version <x>\t\t: select version <x> of algorithm\n");
  fprintf (stderr, "\t-ft\t| --first-touch\t\t: touch memory on different cores\n");
  fprintf (stderr, "\t-h\t| --help\t\t: display help\n");

  exit (val);
}

static void graphics_create_surface (unsigned dim)
{
  Uint32 rmask, gmask, bmask, amask;

  rmask = 0xff000000;
  gmask = 0x00ff0000;
  bmask = 0x0000ff00;
  amask = 0x000000ff;

  DIM = dim;
  image = malloc (dim * dim * sizeof (Uint32));
  alt_image = malloc (dim * dim * sizeof (Uint32));

  if (do_first_touch) {
    void_func_t ft = first_touch [version];
    if (ft != NULL) {
      printf ("(avec first touch)\n");
      ft ();
    } else
      printf ("(pas de first touch disponible pour cette version)\n");
  } else {
    if (first_touch [version] != NULL)
      printf ("(sans first touch)\n");
    else
      printf ("\n");
  }
  
  memset (image, 0, dim * dim * sizeof (Uint32));

  surface = SDL_CreateRGBSurfaceFrom (image, dim, dim, 32,
                                      dim * sizeof (Uint32),
                                      rmask, gmask, bmask, amask);
  if (surface == NULL)
    exit_with_error ("SDL_CreateRGBSurfaceFrom () failed: %s", SDL_GetError ());
}

static void graphics_load_surface (char *filename)
{
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
}

void graphics_image_init (void)
{
  for (int i = 0; i < DIM; i++)
    for (int j = 0; j < DIM; j++)
      if ((cur_img (i, j) & 0xFF) == 0)
	// Si la composante alpha est nulle, on met l'ensemble du pixel à zéro
	cur_img (i, j) = 0;
      else
	cur_img (i, j) |= 0xFF;
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
    } else if (!strcmp (*argv, "--iterations") || !strcmp (*argv, "-i")) {
      if (*argc == 1) {
	fprintf (stderr, "Error: N missing\n");
	usage (1);
      }
      (*argc)--; argv++;
      max_iter = atoi(*argv);
    } else if (!strcmp (*argv, "--refresh-rate") || !strcmp (*argv, "-r")) {
      if (*argc == 1) {
	fprintf (stderr, "Error: N missing\n");
	usage (1);
      }
      (*argc)--; argv++;
      refresh_rate = atoi(*argv);
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

    // Initialisation du moteur de rendu
    ren = SDL_CreateRenderer (win, -1, render_flags);
    if (ren == NULL)
      exit_with_error ("SDL_CreateRenderer");
  }

  printf ("Version utilisée : %s ", version_name [version]);

  if (pngfile == NULL) {
    unsigned size = DIM ? DIM : DEFAULT_DIM;
    graphics_create_surface (size);
    draw_guns ();
  } else
    graphics_load_surface (pngfile);

  graphics_image_init ();

  memcpy (alt_image, image, DIM * DIM * sizeof (Uint32));

  // Création d'une texture à partir de la surface
  //texture = SDL_CreateTextureFromSurface (ren, surface);
  texture = SDL_CreateTexture (ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
			       DIM, DIM);
  PRINT_DEBUG ('g', "DIM = %d\n", DIM);
}

void graphics_share_texture_buffers (void)
{
  GLuint texid;

  SDL_GL_BindTexture (texture, NULL, NULL);

  glGetIntegerv (GL_TEXTURE_BINDING_2D, (GLint *)&texid);

  ocl_map_textures (texid);
}

void graphics_render_image (void)
{
  SDL_Rect src, dst;

  // Refresh texture
  if (opencl_used [version]) {
    
    glFinish ();
    ocl_update_texture ();

  } else {
    SDL_GL_BindTexture (texture, NULL, NULL);

    glTexSubImage2D (GL_TEXTURE_2D,
		     0, /* mipmap level */
		     0, 0, /* x, y */
		     DIM, DIM, /* width, height */
		     GL_RGBA,
		     GL_UNSIGNED_INT_8_8_8_8,
		     image);
  }
  
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
}

void graphics_refresh (void)
{
  // On efface la scène dans le moteur de rendu (inutile !)
  SDL_RenderClear (ren);

  // On réaffiche l'image
  graphics_render_image ();
  
  // Met à jour l'affichage sur écran
  SDL_RenderPresent (ren);
}

void graphics_clean (void)
{
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

  if (texture != NULL)
    SDL_DestroyTexture (texture);

  IMG_Quit ();
  SDL_Quit ();
}

int graphics_display_enabled (void)
{
  return display;
}
