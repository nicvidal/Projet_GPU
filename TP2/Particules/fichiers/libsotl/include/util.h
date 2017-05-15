#ifndef __UTIL_H
#define __UTIL_H

#include <stdlib.h>

char *file_get_contents(const char *filename);

void *xmalloc(size_t size);
void *str_malloc(char *src);

#endif /* __UTIL_H */
