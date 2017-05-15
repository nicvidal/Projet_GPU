#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <omp.h>

#define T 10

int A[T][T];

int k = 0;

void tache(int i,int j)
{
  volatile int x = random() % 1000000;

  for(int z=0; z < x; z++)
    ;

#pragma omp atomic capture
  A[i][j] = k++;
}

int main (int argc, char **argv)
{
  int i, j;

#pragma omp parallel
#pragma omp single
  for (i=0; i < T; i++ )
    for (j=0; j < T; j++ )
      if (i == 0 && j ==0)
        #pragma omp task firstprivate(i,j) depend(out:A[i][j])
        tache(i,j);
      else if (i ==0 && j > 0)
        #pragma omp task firstprivate(i,j) depend(in: A[i][j-1]) depend(out:A[i][j])
        tache(i,j);
      else if (i > 0 && j == 0)
        #pragma omp task firstprivate(i,j) depend(in: A[i-1][j]) depend(out:A[i][j])
        tache(i,j);
      else
        #pragma omp task firstprivate(i,j) depend(in: A[i][j-1]) depend(in: A[i-1][j]) depend(out:A[i][j])
        tache(i,j);

  for (i=0; i < T; i++ ) {
    puts("");
    for (j=0; j < T; j++ )
      printf(" %2d ",A[i][j]) ;
  }

    printf("\n") ;
  return 0;
}
