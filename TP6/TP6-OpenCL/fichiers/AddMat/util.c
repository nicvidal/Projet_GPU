
#include <stdio.h>
#include "util.h"


size_t file_size(const char *filename)
{
  struct stat sb;

  if (stat(filename, &sb) < 0) {
    perror ("stat");
    abort ();
  }
  return sb.st_size;
}

char *file_load(const char *filename)
{
  FILE *f;
  char *b;
  size_t s;
  size_t r;

  s = file_size (filename);
  b = malloc (s+1);
  if (!b) {
    perror ("malloc");
    exit (1);
  }
  f = fopen (filename, "r");
  if (f == NULL) {
    perror ("fopen");
    exit (1);
  }
  r = fread (b, s, 1, f);
  if (r != 1) {
    perror ("fread");
    exit (1);
  }
  b[s] = '\0';
  return b;
}
