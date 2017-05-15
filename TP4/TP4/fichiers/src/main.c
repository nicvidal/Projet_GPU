#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "graphics.h"
#include "compute.h"
#include "error.h"
#include "debug.h"


#include <sys/time.h>
/* macro de mesure de temps, retourne une valeur en µsecondes */
#define TIME_DIFF(t1, t2) \
  ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

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
	if (propager_max [version] () == 0) {
	  stable = 1;
	  printf ("calcul stabilisé en %d itérations\n", iterations);
	} else
	  iterations++;
      }

      graphics_refresh ();
    }
  } else {
    // Version non graphique
    unsigned long temps;
    struct timeval t1, t2;
    
    gettimeofday (&t1,NULL);

    while (!stable) {
      if (propager_max [version] () == 0) {
	stable = 1;
	printf ("calcul stabilisé en %d itérations\n", iterations);
      } else
	iterations++;	
    }
    
    gettimeofday (&t2,NULL);
    
    temps = TIME_DIFF (t1,t2);
    fprintf (stderr, "%ld.%03ld\n", temps / 1000, temps % 1000);

  }

  graphics_clean ();

  return 0;
}
