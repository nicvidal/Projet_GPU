#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <omp.h>

int main()
{
#pragma omp parallel
  {
    int i;
//#pragma omp for schedule(dynamic)
#pragma omp single nowait
  for(i = 0; i < 20 ; i++)
    {
      #pragma omp task firstprivate(i)
      printf("%d par %d \n",i, omp_get_thread_num());
    }

  printf("%d fin \n", omp_get_thread_num());
  }
}
