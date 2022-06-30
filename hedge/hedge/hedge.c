#include <memory.h>
#include <hedge_malloc.h>
#include <stdio.h>
#include "hedge.h"
#include "hedge_internals.h"
#include "lib/debug.h"

#ifndef RELEASE
    #define DPRINT(msg) { printf("<DEBUG> %s\n", #msg); }
    #define DPRINT_LINENUM { printf("<DEBUG> %s %ld\n", __FILE__, __LINE__); }
#else
    #define DPRINT(msg)
    #define DPRINT_LINENUM
#endif

int hedge_set_log_level(int log_level)
{
    set_log_level(log_level);
    return log_level;
}

void hedge_api_compat_test(int testnum, unsigned long arg0, unsigned long arg1)
{
    log_info("hedge_api_test(%d, %lu, %lu)\n", testnum, arg0, arg1);
    switch(testnum) {
        case HEDGE_COMPAT_TEST_LOGGING:
            log_trace("trace log\n");
            log_debug("debug log\n");
            log_info("info log\n");
            log_warning("warning log\n");
            break;
        case HEDGE_COMPAT_TEST_MALLOC:
            if (arg0 == 0)
                arg0 = 1024;
            void *p = hedge_malloc(arg0);
            if (!p) {
                log_fatal("Unable to allocate %d bytes of memory\n", arg0);
            } else {
                hedge_free(p);
            }
            break;
        default:
            log_error("Undefined test\n");
            break;

    }
    log_info("hedge_api_test complete!\n");
}

hedge_ciphertext_t* hedge_encrypt(hedge_t* hedge, double* array, size_t len)
{
    return hedge_internals_encrypt(hedge, array, len);
}

hedge_double_array_t* hedge_decrypt(hedge_t* hedge, hedge_ciphertext_t* ciphertext)
{
    return hedge_internals_decrypt(hedge, ciphertext);
}

hedge_plaintext_t* hedge_encode(hedge_t* hedge, double* array, size_t len)
{
    return hedge_internals_encode(hedge, array, len);
}

hedge_double_array_t* hedge_decode(hedge_t* hedge, hedge_plaintext_t* plaintext)
{
    return hedge_internals_decode(hedge, plaintext);
}

void hedge_shred(hedge_t* hedge)
{
    hedge_internals_shred(hedge);
    hedge_free(hedge);
}

hedge_t* hedge_create(hedge_config_t* config)
{
    log_info("START hedge_create\n");
    hedge_t* hedge = (hedge_t*)hedge_malloc(sizeof(hedge_t));
    log_trace("hedge_create calling hedge_internals_create.\n");
    hedge_internals_create(hedge, config->poly_modulus_degree, 0, 
        config->scale);
    log_trace("hedge_create returned from hedge_internals_create.\n");
    return hedge;
}