#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "graphics.h"
#include "compute.h"
#include "error.h"
#include "debug.h"

int main (int argc, char **argv)
{
  char *debug_flags = NULL;
  int stable = 0;
  int iterations = 0;
  unsigned step;

  debug_init (debug_flags);
  graphics_init (&argc, argv);

  if (graphics_display_enabled ()) {
    // version graphique
    graphics_refresh ();

    for (int quit = 0; !quit;) {

      // Récupération éventuelle des événements clavier, souris, etc.
      step = 1;
      if (debug_enabled ('p'))
	printf ("=== itération %d ===\n", iterations);

      do {
	SDL_Event evt;

	while (SDL_PollEvent (&evt)) {

	  switch (evt.type) {
	  case SDL_QUIT:
	    quit = 1;
	    break;
	  case SDL_KEYDOWN:
	    // Si l'utilisateur appuie sur une touche
	    switch (evt.key.keysym.sym) {
	    case SDLK_ESCAPE:
	      quit = 1;
	      break;
	    case SDLK_SPACE:
	      step = 0;
	      break;
	    default: ;
	    }
	    break ;
	  default: ;
	  }
	}
      } while (debug_enabled ('p') && step && !quit);

      if (!stable) {
	//if (propager_max_v1 () == 0) {
  if (propager_max_v2 () == 0) {
	  stable = 1;
	  printf ("calcul stabilisé en %d itérations\n", iterations);
	} else
	  iterations++;
      }

      graphics_refresh ();
    }
  } else {
    // Version non graphique
    while (!stable) {
      //if (propager_max_v1 () == 0) {
if (propager_max_v2 () == 0) {
	stable = 1;
	printf ("calcul stabilisé en %d itérations\n", iterations);
      } else
	iterations++;
    }
  }

  graphics_clean ();

  return 0;
}
