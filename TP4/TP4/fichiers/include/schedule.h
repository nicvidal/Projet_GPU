#ifndef SCHEDULE_IS_DEF
#define SCHEDULE_IS_DEF


struct job {
  void (*fun)(void *p);
  void *p;
};

extern void task_wait (void);

extern void add_job (struct job todo, int w);

extern void run_workers (int nb);

extern void stop_workers (void);


#endif
