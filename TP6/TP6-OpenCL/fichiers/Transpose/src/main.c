#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "graphics.h"
#include "compute.h"
#include "error.h"
#include "debug.h"
#include "ocl.h"


#include <sys/time.h>
/* macro de mesure de temps, retourne une valeur en µsecondes */
#define TIME_DIFF(t1, t2) \
  ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))


void update_refresh_rate (int p)
{
  static int tab_refresh_rate [] = {1, 2, 5, 10, 100, 1000, 10000, 100000};
  static int i_refresh_rate = 0;
  
  if ((i_refresh_rate == 0 && p < 0) || (i_refresh_rate == 7 && p > 0))
    return;

  i_refresh_rate += p;
  refresh_rate = tab_refresh_rate [i_refresh_rate];
  printf ("\nrefresh rate = %d \n", refresh_rate);
}

int main (int argc, char **argv)
{
  char *debug_flags = NULL;
  int stable = 0;
  int iterations = 0;
  unsigned step;

  debug_init (debug_flags);

  graphics_init (&argc, argv);

  if (opencl_used [version]) {

    ocl_init ();
    ocl_send_image (image);
  }
  
  if (graphics_display_enabled ()) {
    // version graphique

    
    unsigned long temps = 0;
    struct timeval t1, t2;
    
    if (opencl_used [version])
	graphics_share_texture_buffers ();
    
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
	      if (!stable)
		printf ("\nSortie forcée à l'itération %d\n", iterations);
	      quit = 1;
	      break;
	    case SDLK_SPACE:
	      step = 0;
	      break;

	    case SDLK_DOWN :
	      update_refresh_rate(-1);
	      break;
	      
	    case SDLK_UP :
	      update_refresh_rate(1);
	      break;

	    default: ;
	    }
	    break ;

	  default: ;
	  }
	}
      } while (debug_enabled ('p') && step && !quit);

      if (!stable && !quit) {
	if (max_iter && iterations >= max_iter) {
	    if (debug_enabled ('t'))
	      printf ("\nArrêt après %d itérations (durée %ld.%03ld)\n",
		      iterations, temps / 1000  , temps % 1000);
	    else
	      printf ("Arrêt après %d itérations\n", max_iter);
	  stable = 1;
	  graphics_refresh ();
	} else {
	  int n;

	  if (debug_enabled ('t')) {
	    long duree_iteration;

	    gettimeofday (&t1, NULL);
	    n = compute [version] (refresh_rate);
	    ocl_wait ();
	    gettimeofday (&t2, NULL);

	    duree_iteration = TIME_DIFF (t1,t2) ;
	    temps += duree_iteration;
	    int nbiter = (n > 0 ?  n : refresh_rate); 
	    fprintf (stderr,
		     "\r dernière iteration  %ld.%03ld -  temps moyen par itération : %ld.%03ld ",
		     duree_iteration/ nbiter / 1000, (duree_iteration/nbiter) % 1000 ,
		     temps / 1000 / (nbiter+iterations) , (temps/(nbiter+iterations)) % 1000);	
	  } else
	    n = compute [version] (refresh_rate);

	  if (n > 0) {
	    iterations += n;
	    stable = 1;
	    if (debug_enabled ('t'))
	      printf ("\nCalcul terminé en %d itérations (durée %ld.%03ld)\n",
		      iterations, temps / 1000  , (temps) % 1000);
	    else
	      printf ("Calcul terminé en %d itérations\n", iterations);

	  } else
	    iterations += refresh_rate;
	  
	  graphics_refresh ();
	}
      }
    }
  } else {
    // Version non graphique
    unsigned long temps;
    struct timeval t1, t2;
    int n;
    
    gettimeofday (&t1, NULL);

    while (!stable) {
      if (max_iter && iterations >= max_iter) {
	if (opencl_used [version])
	  ocl_wait ();
	printf ("Arrêt après %d itérations\n", max_iter);
	stable = 1;
      } else {
	n = compute [version] (refresh_rate);
	if (n > 0) {
	  iterations += n;
	  stable = 1;
	  printf ("Calcul terminé en %d itérations\n", iterations);
	} else
	  iterations += refresh_rate;
      }
    }

    gettimeofday (&t2, NULL);
    
    temps = TIME_DIFF (t1, t2);
    fprintf (stderr, "%ld.%03ld\n", temps / 1000, temps % 1000);
  }

  graphics_clean ();

  return 0;
}
