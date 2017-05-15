
#ifndef UTIL_H
#define UTIL_H

#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define error(...) do { fprintf(stderr, "Error: " __VA_ARGS__); exit(EXIT_FAILURE); } while(0)
#define check(err, ...)					\
  do {							\
    if(err != CL_SUCCESS) {				\
      fprintf(stderr, "(%d) Error: " __VA_ARGS__ "\n", err);	\
      exit(EXIT_FAILURE);				\
    }							\
  } while(0)

#define TIME_DIFF(t1, t2) \
    ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

size_t file_size(const char *filename);

char *file_load(const char *filename);

#endif
