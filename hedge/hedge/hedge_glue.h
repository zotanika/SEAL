#ifndef __HEDGE_GLUE_H__
#define __HEDGE_GLUE_H__

// Functions declared here MUST be implemented by the developers using lighedge.


// return a (pseudo) random number to be used as a seed to the internal
// random number generator
unsigned long hedge_generate_random_seed(void);

// memory allocator
void * hedge_malloc(size_t size);
void * hedge_calloc(size_t nitems, size_t num);
void * hedge_realloc(void * p, size_t size);
void * hedge_zalloc(size_t size);
void hedge_free(void *p);

// debug functions
int hedge_log(int level, const char* format, ...);

#endif /* __HEDGE_GLUE_H__ */