#ifndef __HEDGE_MALLOC_H__
#define __HEDGE_MALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(ANDROID_NDK)
#include <stdlib.h>
#include <string.h>

void * hedge_malloc(size_t size);
void * hedge_calloc(size_t nitems, size_t num);
void * hedge_realloc(void * p, size_t size);
void * hedge_zalloc(size_t size);
void hedge_free(void *p);

void hedge_mem_print_info(const char* fname);
void hedge_mem_reset_local_stats(void);
size_t hedge_mem_get_curr_alloc(void);

#else
#include <malloc.h>

#define hedge_malloc(size)      malloc(size)
#define hedge_free(p)           free(p)
#define hedge_calloc(num, size) calloc(num, size)
#define hedge_realloc(p, size)  realloc(p, size)
#define hedge_zalloc(size) ({void *p = malloc(size); \
                             if (p != NULL)  memset(p, 0, size); \
                             p;})
#endif
#ifdef __cplusplus
}
#endif

#endif /*__HEDGE_MALLOC_H__*/