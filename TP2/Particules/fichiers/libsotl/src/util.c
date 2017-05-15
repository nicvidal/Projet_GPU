#include "sotl.h"
#include "util.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static long file_size(FILE *f)
{
    long pos, end;

    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, pos, SEEK_SET);

    return end;
}

 char *file_get_contents(const char *filename)
{
    FILE *f;
    char *b;
    long s;
    size_t r;

    if (!(f = fopen(filename, "r"))) {
        perror("fopen");
        return NULL;
    }

    s = file_size(f);
    if (!(b = malloc(s + 1))) {
        perror("malloc");
        goto fail;
    }

    if ((r = fread(b, s, 1, f)) != 1) {
        perror("fread");
        goto fail;
    }

    b[s] = '\0';
    fclose(f);
    return b;

fail:
    fclose(f);
    free(b);
    return NULL;
}

void *xmalloc(size_t size)
{
    void *ptr = NULL;

    if (!(ptr = malloc(size)))
        sotl_log(CRITICAL, "%s\n", strerror(errno));
    return ptr;
}

void *str_malloc(char *src)
{
  char *ptr = xmalloc (strlen (src) + 1);
  strcpy (ptr, src);
  return ptr;
}
