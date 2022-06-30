#ifndef __HEDGE_INTERNALS_H__
#define __HEDGE_INTERNALS_H__

#include "hedge.h"
#include "lib/list.h"

void hedge_internals_create(hedge_t* hedge, uint64 poly_mod_degree, 
    uint64 coeff_mod_cnt, double scale);

hedge_ciphertext_t* hedge_internals_encrypt(hedge_t* hedge, double* array, 
    size_t count);

hedge_double_array_t* hedge_internals_decrypt(hedge_t* hedge, 
    hedge_ciphertext_t* ciphertext);

hedge_plaintext_t* hedge_internals_encode(hedge_t* hedge, double* array, 
    size_t count);

hedge_double_array_t* hedge_internals_decode(hedge_t* hedge, 
    hedge_plaintext_t* plaintext);

void hedge_internals_shred(hedge_t* hedge);

#endif /* __HEDGE_INTERNALS_H__ */