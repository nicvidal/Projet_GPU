
#include "compute.h"
#include "graphics.h"
#include "debug.h"
#include "schedule.h"

#define DEFAULT_NUM_THREADS 4

unsigned version = 0;

void prelude_v4 (void);

void postlude_v4 (void);

void first_touch_omp (void);
void first_touch_v4 (void);

int propager_max_v0 (void);
int propager_max_v1 (void);
int propager_max_v2 (void);
int propager_max_v3 (void);
int propager_max_v4 (void);

void_func_t prelude [] = {
  NULL,
  NULL,
  NULL,
  NULL,
  prelude_v4
};

void_func_t postlude [] = {
  NULL,
  NULL,
  NULL,
  NULL,
  postlude_v4
};

void_func_t first_touch [] = {
  first_touch_omp,
  first_touch_omp,
  first_touch_omp,
  first_touch_omp,
  first_touch_v4
};

int_func_t propager_max [] = {
  propager_max_v0,
  propager_max_v1,
  propager_max_v2,
  propager_max_v3,
  propager_max_v4
};

char *version_name [] = {
  "Propagation séquentielle simple",
  "Propagation séquentielle par tuiles",
  "Propagation OpenMP avec omp for",
  "Propagation OpenMP avec tâches",
  "Propagation avec ordonnanceur maison"
};

///////////////////////////// Version séquentielle simple

// NE PAS MODIFIER

int monter_max_seq (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  for (int i = i_d; i >= i_f; i--)
    for (int j = j_d; j >= j_f; j--)
      if (img (i,j)) {
	Uint32 m = MAX(img (i+1,j), img (i,j+1));
	if (m > img (i,j)) {
	  changement = 1;
	  img (i,j) = m;
	}
      }

  return changement;
}

int descendre_max_seq (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++)
      if (img (i,j)) {
	Uint32 m = MAX(img (i-1,j), img (i,j-1));
	if (m > img (i,j)) {
	  changement = 1;
	  img (i,j) = m;
	}
      }

  return changement;
}

int propager_max_v0 (void)
{
  return monter_max_seq (DIM-2, DIM-2, 0, 0) | descendre_max_seq (1, 1, DIM-1, DIM-1);
}

///////////////////////////// Version séquentielle par tuiles

#define GRAIN 32

// NE PAS MODIFIER (sauf GRAIN)

volatile int celluled[GRAIN][GRAIN+1];
volatile int cellulem[GRAIN][GRAIN+1];

volatile int cont = 0;

unsigned tranche = 0;

void lancer_descente (int i, int j)
{
  int i_d = (i == 0) ? 1 : i * tranche;
  int j_d = (j == 0) ? 1 : j * tranche;
  int i_f = (i == GRAIN-1) ? DIM-1 : (i+1) * tranche - 1;
  int j_f = (j == GRAIN-1) ? DIM-1 : (j+1) * tranche - 1;

  PRINT_DEBUG ('c', "Descente: bloc(%d,%d) couvre (%d,%d)-(%d,%d)\n", i, j, i_d, j_d, i_f, j_f);

  if (descendre_max_seq (i_d, j_d, i_f, j_f))
    cont = 1;
}

void lancer_monte (int i, int j)
{

  int i_d = (i == GRAIN-1) ? DIM-2 : (i+1) * tranche - 1;
  int j_d = (j == GRAIN-1) ? DIM-2 : (j+1) * tranche - 1;
  int i_f = i * tranche;
  int j_f = j * tranche;

  PRINT_DEBUG ('c', "Montée: bloc(%d,%d) couvre (%d,%d)-(%d,%d)\n", i, j, i_d, j_d, i_f, j_f);

  if (monter_max_seq (i_d, j_d, i_f, j_f))
    cont = 1;
}

int propager_max_v1 (void)
{
  tranche = DIM / GRAIN;

  cont = 0;

  for (int i=0; i < GRAIN; i++)
    for (int j=0; j < GRAIN; j++)
      lancer_monte (GRAIN-i-1, GRAIN-j-1);

  for (int i=0; i < GRAIN; i++)
    for (int j=0; j < GRAIN; j++)
      lancer_descente (i, j);

  return cont;
}

///////////////////////////// Version OpenMP avec omp for

void first_touch_omp ()
{
  int i,j ;

  for(i=0; i<DIM ; i++) {
    for(j=0; j < DIM ; j += 512)
      img (i, j) = 0 ;
  }
}

int monter_max (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  #pragma omp parallel for
  for (int i = i_d; i >= i_f; i--)
    for (int j = j_d; j >= j_f; j--)
      if (img (i,j)) {
	Uint32 m = MAX(img (i+1,j), img (i,j+1));
	if (m > img (i,j)) {
	  changement = 1;
	  img (i,j) = m;
	}
      }

  return changement;
}

int descendre_max (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  #pragma omp parallel for
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++)
      if (img (i,j)) {
	Uint32 m = MAX(img (i-1,j), img (i,j-1));
	if (m > img (i,j)) {
	  changement = 1;
	  img (i,j) = m;
	}
      }

  return changement;
}

int propager_max_v2 (void)
{
  return monter_max (DIM-2, DIM-2, 0, 0) | descendre_max (1, 1, DIM-1, DIM-1);
}

///////////////////////////// Version OpenMP avec tâches
int propager_max_v3 (void)
{
  tranche = DIM / GRAIN;
  cont = 0;

  #pragma omp parallel
  {
  #pragma omp single nowait
  for (int i=0; i < GRAIN; i++) {
    for (int j=0; j < GRAIN; j++) {
      if (i == 0 && j ==0)
        #pragma omp task firstprivate(i,j) depend(out:cellulem[GRAIN-i-1][GRAIN-j-1])
        lancer_monte(GRAIN-i-1, GRAIN-j-1);
      else if (i ==0 && j > 0)
        #pragma omp task firstprivate(i,j) depend(in: cellulem[GRAIN-i-1][GRAIN-j-2]) depend(out:cellulem[GRAIN-i-1][GRAIN-j-1])
        lancer_monte(GRAIN-i-1, GRAIN-j-1);
      else if (i > 0 && j == 0)
        #pragma omp task firstprivate(i,j) depend(in: cellulem[GRAIN-i-2][GRAIN-j-1]) depend(out:cellulem[GRAIN-i-1][GRAIN-j-1])
        lancer_monte(GRAIN-i-1, GRAIN-j-1);
      else
        #pragma omp task firstprivate(i,j) depend(in: cellulem[GRAIN-i-1][GRAIN-j-2]) depend(in: cellulem[GRAIN-i-2][GRAIN-j-1]) depend(out:cellulem[GRAIN-i-1][GRAIN-j-1])
       	lancer_monte(GRAIN-i-1, GRAIN-j-1);
    }
}
  #pragma omp single nowait
  for (int i=0; i < GRAIN; i++) {
    for (int j=0; j < GRAIN; j++) {
	if (i == 0 && j ==0)
        #pragma omp task firstprivate(i,j) depend(out:celluled[i][j])
        lancer_descente (i, j);
      else if (i ==0 && j > 0)
        #pragma omp task firstprivate(i,j) depend(in: celluled[i][j-1]) depend(out:celluled[i][j])
        lancer_descente (i, j);
      else if (i > 0 && j == 0)
        #pragma omp task firstprivate(i,j) depend(in: celluled[i-1][j]) depend(out:celluled[i][j])
        lancer_descente (i, j);
      else
        #pragma omp task firstprivate(i,j) depend(in: celluled[i][j-1]) depend(in: celluled[i-1][j]) depend(out:celluled[i][j])
       	lancer_descente (i, j);
    }
}
   #pragma taskwait
  }
  return cont;
}

///////////////////////////// Version utilisant un ordonnanceur maison

unsigned P;

void ajouter_job (int i, int j, int type);
void lancer_descente_job (int i, int j);
void lancer_monte_job (int i, int j);
void lancer_first_touch_job (int i, int j);

// Attention, à ce point de l'exécution, on ne connaît pas encore DIM...
void prelude_v4 ()
{
  char *str = getenv ("OMP_NUM_THREADS");

  if (str == NULL)
    P = DEFAULT_NUM_THREADS;
  else
    P = atoi(str);

  PRINT_DEBUG('t', "[Démarrage des %d workers]\n", P);
  run_workers(P);
}

void postlude_v4()
{
  stop_workers ();
  PRINT_DEBUG ('t', "[Arrêt des workers]\n");
}

void first_touch_v4(void)
{
  tranche = DIM / GRAIN;

  lancer_first_touch_job (0,0);
  task_wait ();
}

static inline void *pack_args(int i, int j)
{
  uint64_t x = (uint64_t)i << 32 | j;
  return (void *)x;
}

static inline void unpack_args(void *a, int *i, int *j)
{
  *i = (uint64_t)a >> 32;
  *j = (uint64_t)a & 0xFFFFFFFF;
}

void zero_seq(int i_d, int j_d, int i_f, int j_f)
{
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++)
      img (i,j) = 0;
}

void lancer_first_touch (int i, int j)
{
  int i_d = (i == 0) ? 1 : i * tranche;
  int j_d = (j == 0) ? 1 : j * tranche;
  int i_f = (i == GRAIN-1) ? DIM-1 : (i+1) * tranche - 1;
  int j_f = (j == GRAIN-1) ? DIM-1 : (j+1) * tranche - 1;

  PRINT_DEBUG ('f', "First-Touch: bloc(%d,%d) couvre (%d,%d)-(%d,%d)\n", i, j, i_d, j_d, i_f, j_f);

  zero_seq (i_d, j_d, i_f, j_f);
}

void first_touch_job (void *p)
{
  int ii, jj;

  unpack_args (p, &ii, &jj);

  lancer_first_touch (ii, jj);

  if (jj < GRAIN-1)
    lancer_first_touch_job (ii, jj+1);

  if (ii < GRAIN-1)
    lancer_first_touch_job (ii+1, jj);
}

void monter_max_job (void *p)
{
  int ii, jj;

  unpack_args (p, &ii, &jj);

  lancer_monte (ii, jj);

  if (jj > 0)
    lancer_monte_job (ii, jj-1);

  if (ii > 0)
    lancer_monte_job (ii-1, jj);

}

void descendre_max_job (void *p)
{
  int ii, jj;

  unpack_args (p, &ii, &jj);

  lancer_descente (ii, jj);

  if (jj < GRAIN-1)
    lancer_descente_job (ii, jj+1);

  if (ii < GRAIN-1)
    lancer_descente_job (ii+1, jj);
}

void lancer_descente_job (int i, int j)
{
  // celluled gere les dependances entre blocs pour la descente
  // celluled[x][y] == 0 <==> les blocs [x-1][y] et [x][y-1] ne sont pas termines
  // celluled[x][y] == 1 <==> un des blocs [x-1][y] et [x][y-1] a termine
  // celluled[x][y] == 2 <==> les blocs [x-1][y] et [x][y-1] sont termines

  int val = __sync_add_and_fetch (&celluled [i][j], 1);

  if ( val != 2 && i != 0 && j != 0)
    return;

  celluled [i][j] = 0;
  ajouter_job (i, j, 0);
}

void lancer_monte_job (int i, int j)
{
  int val = __sync_add_and_fetch (&cellulem [i][j], 1);

  if (val !=2 && i != GRAIN-1 && j != GRAIN-1)
    return;

  cellulem [i][j] = 0;
  ajouter_job (i, j, 1);
}

void lancer_first_touch_job (int i, int j)
{
  int val = __sync_add_and_fetch (&celluled [i][j], 1);

  if ( val != 2 && i != 0 && j != 0)
    return;

  celluled [i][j] = 0;
  ajouter_job (i, j, 2);
}


int propager_max_v4 (void)
{
  tranche = DIM / GRAIN;

  cont = 0;

  lancer_monte_job (GRAIN-1, GRAIN-1);
  task_wait ();

  lancer_descente_job (0, 0);
  task_wait ();

  return cont;
}

void ajouter_job (int i, int j, int type)
{
  struct job todo;
  int coeur = i % P;

  todo.p = pack_args (i, j);

  if (type == 0)
    todo.fun = descendre_max_job;
  else if (type == 1)
    todo.fun = monter_max_job;
  else
    todo.fun = first_touch_job;

  add_job (todo, coeur);
}
