#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hedge_malloc.h"
#include "lib/debug.h"

#if defined(ANDROID_NDK)
#else
#if !defined(ARCH_ARM) /* #if defined(__i386__) || defined(__x86_64__) */

#include <execinfo.h>

#ifdef _GNU_SOURCE
#include <dlfcn.h>
#endif

#define ALLOCINFO_MAGIC 0x464E49434F4C4C41
#define ALLOCINFO_GUARD 0xEEEEEEEEEEEEEEEE
#define MALLOC_STACK_SIZE 8

#define ALLOCINFO_GUARD_COUNT 4
struct allocinfo {
    unsigned long guard[ALLOCINFO_GUARD_COUNT];
    unsigned long magic;
    size_t size;
    struct allocinfo *prev;
    struct allocinfo *next;
    void *stack[MALLOC_STACK_SIZE];
};

#define ALLOCINFO_SZ sizeof(struct allocinfo)

extern void *__real_malloc(size_t size);
extern void __real_free(void *p);
extern void *__real_calloc(size_t nitems, size_t size);
extern void *__real_realloc(void* p, size_t size);


// Keep in mind that memory stats are not thread safe
struct memstats {
    size_t total;
    size_t total_with_allocinfo;
    size_t current;
    size_t peak;
};
static struct memstats global_memstats;
static struct memstats local_memstats;


struct allocinfo g_node_list_head;

#define HEDGE_MEM_DEBUG
#ifdef HEDGE_MEM_DEBUG


// resets the local memory stats
void hedge_mem_reset_local_stats(void)
{
    local_memstats.total = 0;
    local_memstats.total_with_allocinfo = 0;
    local_memstats.current = 0;
    local_memstats.peak = 0;
}

static void add_node(struct allocinfo *node)
{
    node->next = g_node_list_head.next;
    if (node->next)
        node->next->prev = node;
    node->prev = &g_node_list_head;
    g_node_list_head.next = node;
    //printf("... %p ..\n", node);
}

static void remove_node(struct allocinfo *node)
{
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
}

static void update_meminfo(int size)
{
    global_memstats.current += size;
    if (global_memstats.current > global_memstats.peak)
        global_memstats.peak = global_memstats.current;
    local_memstats.current += size;
    if (local_memstats.current > local_memstats.peak)
        local_memstats.peak = local_memstats.current;
}

#ifdef _GNU_SOURCE
#warning "_GNU_SOURCE is defined"
#else
#warning "_GNU_SOURCE is NOT defined"
#endif

size_t hedge_mem_get_curr_alloc(void)
{
    return global_memstats.current;
}

void hedge_mem_print_info(const char* fname)
{
#ifdef _GNU_SOURCE
    Dl_info symbol_info;
#endif
    struct allocinfo *p = &g_node_list_head;
    size_t total_size = 0;
    size_t count = 0;
    int i;

    FILE* output = stdout;
    if (fname) {
        output = fopen(fname, "w+");
        if (!output) output = stdout;
    }
    // print local info
    fprintf(output, "============= Memory Info (Since User Reset) ============\n");
    fprintf(output, "Accumulated allocation : %lu bytes\n", local_memstats.total);
    fprintf(output, "Current allocation : %lu bytes\n", local_memstats.current);
    fprintf(output, "Peak allocation : %zu\n\n", local_memstats.peak);

    fprintf(output, "============= Memory Info (Since Inception) ============\n");
    fprintf(output, "Accumulated allocation : %lu bytes\n", global_memstats.total);
    fprintf(output, "Current allocation : %lu bytes\n", global_memstats.current);
    fprintf(output, "Peak allocation : %zu\n\n", global_memstats.peak);

    while(p) {
        fprintf(output, "user addr:%p size:%zu (node:%p)", (void*)p + ALLOCINFO_SZ, p->size, p);
        fprintf(output, " caller: ");
        for (i = 2 ;i < MALLOC_STACK_SIZE; i++) {
            if (!p->stack[i]) break;
#ifdef _GNU_SOURCE
            dladdr(p->stack[i], &symbol_info);
            fprintf(output, "%s (%p)    ", symbol_info.dli_sname, p->stack[i]);
#else
            fprintf(output, "%p   ", p->stack[i]);
#endif
        }
        fprintf(output, "\n");
        count++;
        total_size += p->size;

        p = p->next;
    }
    fprintf(output, "=========================================\n");
    fprintf(output, "Total allocation : %zu\n", count);
    fprintf(output, "Total allocated size : %zu bytes\n", total_size);
}
void *set_alloc_info(void *p, size_t size)
{
    struct allocinfo *minfo = (struct allocinfo *)p;
    minfo->magic = ALLOCINFO_MAGIC;
    minfo->size = size;
    minfo->prev = NULL;
    minfo->next = NULL;
    int n = backtrace(minfo->stack, MALLOC_STACK_SIZE);
    int i;
    for (i = 0; i < ALLOCINFO_GUARD_COUNT; i++) {
        minfo->guard[i] = ALLOCINFO_GUARD;
    }

    global_memstats.total += size;
    global_memstats.total_with_allocinfo += (size + ALLOCINFO_SZ);
    local_memstats.total += size;
    local_memstats.total_with_allocinfo += (size + ALLOCINFO_SZ);

    //printf("p =%p newp=%p (size=%zu)\n", p, p + ALLOCINFO_SZ, size);
    return (void *)(p + ALLOCINFO_SZ);
}

void *get_alloc_ptr(void *p)
{
    struct allocinfo *info = (struct allocinfo *)(p - ALLOCINFO_SZ);
    if (info->magic == ALLOCINFO_MAGIC && info->guard[2] == ALLOCINFO_GUARD) {
        //printf("Allocated by %p (size=%zu)\n", info->stack[0], info->size);
        return (void *)info;
    } else {
        int i;
        for (i = 0; i < ALLOCINFO_GUARD_COUNT; i++)
            log_fatal("guard[%d] = %p\n", i, info->guard[i]);
        log_fatal("guard[0] = %lx\n", info->guard);
        log_fatal("magic = %lx\n", info->magic);
        log_fatal("size = %lx\n", info->size);
        for (i = 0; i < MALLOC_STACK_SIZE; i++)
            log_fatal("stack[%d] = %p\n", i, info->stack[i]);
    }
    return p; 
}

void *__wrap_malloc(size_t size)
{
    void *realp = __real_malloc(size + ALLOCINFO_SZ);
    void *p = set_alloc_info(realp, size);
    update_meminfo(((struct allocinfo *)realp)->size);
    add_node(realp);
    return p;
}

void __wrap_free(void *p)
{
    if (!p) {
#ifdef HEDGE_DEBUG
        log_warning("Trying to free null pointer!\n");
        dumpstack();
#endif
        return;
    }
    void *realp = get_alloc_ptr(p);
    update_meminfo(-((struct allocinfo *)realp)->size);
    remove_node(realp);
    //log_info("free: p=%p realp=%p\n", p, realp);
    //dumpstack_simple();
    __real_free(realp);
    
    //global_memstats.total -= 
}

void *__wrap_calloc(size_t nitems, size_t size)
{
    void *realp = __real_calloc(1, nitems * size + ALLOCINFO_SZ);
    void *p = set_alloc_info(realp, size);
    update_meminfo(((struct allocinfo *)realp)->size);
    add_node(realp);
    return p;
}

void *__wrap_realloc(void* p, size_t size)
{
    void *newrealp = __real_malloc(size + ALLOCINFO_SZ);
    void *newp = set_alloc_info(newrealp, size);
    void *oldrealp = get_alloc_ptr(p);
    update_meminfo(((struct allocinfo *)newrealp)->size);
    add_node(newrealp);
    memcpy(newp, p, size);
    __wrap_free(p);
    return newp;
}
#else

void *__wrap_malloc(size_t size)
{
    return __real_malloc(size);
}

void __wrap_free(void *p)
{
    return __real_free(p);
    //global_memstats.total -= 
}

void *__wrap_calloc(size_t nitems, size_t size)
{
    return __real_calloc(nitems, size);
}

void *__wrap_realloc(void* p, size_t size)
{
    return __real_realloc(p, size);
}
#endif

#endif

void __attribute__((weak)) * hedge_malloc(size_t size)
{
    return malloc(size);
}

void __attribute__((weak)) hedge_free(void * p)
{
    free(p);
}

void __attribute__((weak)) * hedge_realloc(void * p, size_t size)
{
    return realloc(p, size);
}

void __attribute__((weak)) * hedge_calloc(size_t nitems, size_t size)
{
    return calloc(nitems, size);
}

void __attribute__((weak)) * hedge_zalloc(size_t size)
{
    void *p = hedge_malloc(size);
    if (p != NULL)
        memset(p, 0, size);
    return p;
}
#endif