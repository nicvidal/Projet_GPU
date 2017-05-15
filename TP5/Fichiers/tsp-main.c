#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <sys/time.h>

// #include <omp.h>

#define MAXE	30

typedef int DTab_t [MAXE] [MAXE];

/* macro de mesure de temps, retourne une valeur en �secondes */
#define TIME_DIFF(t1, t2) \
        ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))



/* variables globales */

/* arguments du programme */
int    Argc ;
char   **Argv ;

/* dernier minimum trouv� */
int    minimum = INT_MAX;

/* tableau des distances */
DTab_t    distance ;

/* nombre de villes */
int NrTowns ;

/* profondeur du parall�lisme */
int grain;


#define MAXX	100
#define MAXY	100
typedef struct
		{
		 int x, y ;
		} coor_t ;

typedef coor_t coortab_t [MAXE] ;
coortab_t towns ;

/* initialisation du tableau des distances */
void genmap ()
{
int i, j ;
int dx, dy;

 NrTowns = atoi (Argv[1]) ;
 if (NrTowns > MAXE) {
  fprintf(stderr,"trop de villes, augmentez MAXE dans tsp-types.h");
  exit(1);
 }

 srand (atoi(Argv[2])) ;

 for (i=0; i<NrTowns; i++)
  {
   towns [i].x = rand () % MAXX ;
   towns [i].y = rand () % MAXY ;
  }

 for (i=0; i<NrTowns; i++)
  {
   for (j=0; j<NrTowns; j++)
    {
     /* Un peu r�aliste */
     dx = towns [i].x - towns [j].x ;
     dy = towns [i].y - towns [j].y ;
     distance [i][j] = (int) sqrt ((double) ((dx * dx) + (dy * dy))) ;
    }
  }
}


/* impression tableau des distances, pour v�rifier au besoin */
void PrintDistTab ()
{
 int i, j ;

 printf ("NrTowns = %d\n", NrTowns) ;

 for (i=0; i<NrTowns; i++)
  {
   printf ("distance [%1d]",i) ;
   for (j=0; j<NrTowns; j++)
    {
     printf (" [%2d:%2d] ", j, distance[i][j]) ;
    }
   printf (";\n\n") ;
  }
 printf ("done ...\n") ;

}

void printPath (int *path)
{
 char toprint[MAXY][MAXX+1];
 int i;
 int x, y;

 memset(toprint, ' ', sizeof(toprint));
 for (i = 0; i < NrTowns-1; i++)
 {
  int x1 = towns[path[i]].x;
  int y1 = towns[path[i]].y;
  int x2 = towns[path[i+1]].x;
  int y2 = towns[path[i+1]].y;

  if (abs(x2-x1) > abs(y2-y1))
   for (x = 1; x < abs(x2-x1); x++) {
    toprint[y1+x*(y2-y1)/abs(x2-x1)][x1+x*(x2-x1)/abs(x2-x1)] = '*';
   }
  else
   for (y = 1; y < abs(y2-y1); y++) {
    toprint[y1+y*(y2-y1)/abs(y2-y1)][x1+y*(x2-x1)/abs(y2-y1)] = '*';
   }
 }

 for (i = 0; i < NrTowns; i++)
  toprint[towns[i].y][towns[i].x] = '#';
 for (y = 0; y < MAXY; y++) {
  toprint[y][MAXX] = 0;
  printf("%s\n", toprint[y]);
 }
}

/* résolution du problème du voyageur de commairce */

inline int present (int city, int hops, int mask)
{

 return mask & (1 << city) ;

}
/* ancien, séquentiel
void tsp (int hops, int len, int *path, int mask)
{
 int i ;
 int me, dist ;

 if (hops == NrTowns)
   {
     if (len +  distance[0][path[NrTowns-1]]< minimum)
       {
	 minimum = len +  distance[0][path[NrTowns-1]];
	 //printf ("found path len = %3d :", minimum) ;
	 //for (i=0; i < NrTowns; i++)
	   //printf ("%2d ", path[i]) ;
	 //printf ("\n") ;
      }
   }
 else
   {
     me = path [hops-1] ;

     #pragma omp parallel for if (hops <= grain)
     for (i=0; i < NrTowns; i++)
       {
	        if (!present (i, hops, mask))
	         {
	            path [hops] = i ;
	            dist = distance[me][i] ;
	            tsp (hops+1, len+dist, path,  mask | (1 << i)) ;
	          }
       }
   }
}*/

void tsp (int hops, int len, int *path, int mask)
{
 int i ;
 int me, dist ;

 if (hops == NrTowns) {
     if (len +  distance[0][path[NrTowns-1]]< minimum) {
         #pragma omp critical
         if (len +  distance[0][path[NrTowns-1]]< minimum) {
    	     minimum = len +  distance[0][path[NrTowns-1]];
    	     //printf ("found path len = %3d :", minimum) ;
    	     //for (i=0; i < NrTowns; i++)
    	     //printf ("%2d ", path[i]) ;
    	     //printf ("\n") ;
        }
     }
  }
else
  {
    me = path[hops-1] ;

    #pragma omp parallel for if (hops <= grain)
    for (i=0; i < NrTowns; i++)
      {
         if (!present (i, hops, mask))
          {
             int chemin[MAXE];
             memcpy(chemin, path, hops*sizeof(int));
             chemin[hops] = i;
             dist = distance[me][i] ;
             tsp(hops+1, len+dist, path,  mask | (1 << i)) ;
           }
      }
  }
}

int main (int argc, char **argv)
{
   unsigned long temps;
   struct timeval t1, t2;
   int path[MAXE];

   if (argc < 3 || argc > 4)
     {
	      fprintf (stderr, "Usage: %s  <ncities> <seed> [grain]\n",argv[0]) ;
	      exit(1) ;
     }

   if (argc ==4)
    grain = atoi(argv[3]);
  else
    grain = 0;

   Argc = argc ;
   Argv = argv ;
   minimum = INT_MAX ;

   //printf ("ncities = %3d\n", atoi(argv[1])) ;

   genmap () ;

   gettimeofday(&t1,NULL);

   path [0] = 0;

   tsp(1,0,path,1);

   gettimeofday(&t2,NULL);

   temps = TIME_DIFF(t1,t2);

   printf("time = %ld.%03ldms\n", temps/1000, temps%1000);

   return 0 ;
}
