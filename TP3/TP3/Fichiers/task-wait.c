#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <omp.h>


void creer_tache(char *nom)
{
  int maman = omp_get_thread_num();
#pragma omp task firstprivate(maman)
      {
	printf("%s [fille de %d] executee par %d \n",nom,maman,omp_get_thread_num());
	sleep(1);
      }
}


int main()
{
#pragma omp parallel
  {
#pragma omp single nowait
    {
      printf("thread %d va créer A et B \n", omp_get_thread_num());
      creer_tache("A");
      creer_tache("B");
    }

#pragma omp single nowait
    {
      printf("thread %d va créer C et D \n", omp_get_thread_num());
      creer_tache("C");
      creer_tache("D");
    }

    printf("thread %d approche de taskwait \n", omp_get_thread_num());
#pragma omp barrier
    printf("thread %d a passe taskwait \n", omp_get_thread_num());
  }
  return 0;
}
