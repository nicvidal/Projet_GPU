
#define _GNU_SOURCE  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifdef __linux__
#include <sched.h>
#endif

#include "schedule.h"
#include "debug.h"

static int nbWorkers;

volatile static int nbTask = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


void task_wait()
{
  pthread_mutex_lock (&mutex);
  while (nbTask > 0)
    pthread_cond_wait (&cond,&mutex);
  pthread_mutex_unlock (&mutex);
}

void one_more_task()
{
  pthread_mutex_lock (&mutex);
  nbTask++;
  pthread_mutex_unlock (&mutex);
}

void one_less_task()
{
  pthread_mutex_lock (&mutex);
  nbTask--;
  if (nbTask == 0)
     pthread_cond_signal (&cond);
  pthread_mutex_unlock (&mutex);
}

struct worker
{
  int id;
  pthread_t tid;
  pthread_attr_t attr;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  int nbjobs;
  int fin;
  struct job jobs[100];
} *workers;


void add_job (struct job todo, int w)
{
  one_more_task ();
  pthread_mutex_lock (&workers[w].mutex);
  workers[w].jobs[workers[w].nbjobs++] = todo;
  pthread_cond_broadcast (&workers[w].cond);
  pthread_mutex_unlock (&workers[w].mutex);
}

void no_more_job (int w)
{
  pthread_mutex_lock (&workers[w].mutex);
  workers[w].fin = 1;
  pthread_cond_signal (&workers[w].cond);
  pthread_mutex_unlock (&workers[w].mutex);
}

void *worker_main (void *p)
{
  struct worker *me = (struct worker *) p;
  struct job todo;
  unsigned tasks = 0;
  
  PRINT_DEBUG ('t', "Hey, I'm worker %d\n", me->id);
  
  while (1) {
    
    pthread_mutex_lock (&me->mutex);
    
    if (me->nbjobs == 0 && me->fin == 0)
      pthread_cond_wait (&me->cond, &me->mutex);
    
    if (me->nbjobs > 0) {  
      todo=me->jobs[0];
      me->nbjobs--;
      if (me->nbjobs) // une FIFO a l'arrache
	memmove (me->jobs, &(me->jobs[1]), me->nbjobs * sizeof(struct job));
    } else if (me->fin == 1)
      me->fin = -1;
    
    pthread_mutex_unlock(&me->mutex);
    
    if (me->fin == -1) {
      PRINT_DEBUG ('t', "Worker %d has computed %d tasks\n", me->id, tasks);
      return NULL;
    }

    tasks++;
    todo.fun (todo.p);
    one_less_task ();
  }
}


void run_workers (int nb)
{
  int i;
  
  nbWorkers = nb;
  
  workers = malloc (nbWorkers * sizeof (struct worker));

  for (i=0; i < nbWorkers; i++) {
    workers[i].id = i;
    workers[i].fin = 0;
    workers[i].nbjobs = 0;
    pthread_cond_init (&workers[i].cond, NULL);
    pthread_mutex_init (&workers[i].mutex, NULL);
    pthread_attr_init (&workers[i].attr);
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO (&cpuset);
    CPU_SET (i, &cpuset);
    pthread_attr_setaffinity_np (&workers[i].attr, sizeof (cpuset), &cpuset);
#endif      

    pthread_create (&workers[i].tid, &workers[i].attr, worker_main, &workers[i]);
  }
}

void stop_workers (void)
{
  int i;
  
  for (i=0; i < nbWorkers; i++)
    no_more_job (i);  
  
  for (i=0; i < nbWorkers; i++)
    pthread_join (workers[i].tid, NULL);

  free (workers);
}

