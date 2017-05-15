
#ifndef COMPUTE_IS_DEF
#define COMPUTE_IS_DEF

typedef void (*void_func_t) (void);
typedef int (*int_func_t) (void);

extern void_func_t first_touch [];
extern int_func_t propager_max [];
extern char *version_name [];
extern void_func_t prelude [];
extern void_func_t postlude [];

extern unsigned version;


#endif
