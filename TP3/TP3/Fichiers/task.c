#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <omp.h>

void generer()
{
  char chaine[]="jusqu'ici tout va bien";

  for(int i = 0; i < 10 ; i++)
#pragma omp task shared(chaine) firstprivate(i)
    printf("execution de la tache %d par le thread %d >>>> %s <<<< \n",i,omp_get_thread_num(), chaine);

#pragma omp taskwait
}

int main()
{

#pragma omp parallel
  {
#pragma omp master
    generer();
    printf("%d approche de la barriere implicite \n", omp_get_thread_num());
  }
  return 0;
}
