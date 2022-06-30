#ifndef __ENCRYPTIONPARAMS_H__
#define __ENCRYPTIONPARAMS_H__

#include <complex.h>
#include <memory.h>
#include "math/rng.h"
#include "math/smallmodulus.h"
#include "lib/vector.h"
#include "lib/hash.h"
#include "lib/bytestream.h"

typedef enum {
    BFV = 0x1,
    CKKS = 0x2
} scheme_type;


typedef struct encrypt_parameters encrypt_parameters;

typedef sha3_block_type parms_id_type;

struct encrypt_parameters{
    void (*set_poly_modulus_degree)(encrypt_parameters* self, size_t poly_modulus_degree);
    void (*set_coeff_modulus)(encrypt_parameters* self, vector(Zmodulus)* coeff_modulus);
    void (*set_plain_modulus)(encrypt_parameters* self, const Zmodulus* plain_modulus);
    void (*set_plain_modulus_uint64)(encrypt_parameters* self, uint64_t plain_modulus);
    scheme_type (*scheme)(encrypt_parameters* self);
    size_t (*poly_modulus_degree)(encrypt_parameters* self);
    vector(Zmodulus)* (*coeff_modulus)(encrypt_parameters* self);
    Zmodulus* (*plain_modulus)(encrypt_parameters* self) ;
    void (*save)(encrypt_parameters* self, bytestream_t* stream);
    void (*load)(encrypt_parameters* self, bytestream_t* stream);
    bool (*is_valid_scheme)(encrypt_parameters* self, uint8_t scheme);
    parms_id_type* (*parms_id)(encrypt_parameters* self);
    void (*compute_parms_id)(encrypt_parameters* self);

    scheme_type scheme_;
    size_t poly_modulus_degree_;
    vector(Zmodulus)* coeff_modulus_;
    Zmodulus* plain_modulus_;
    parms_id_type parms_id_;
};

extern parms_id_type parms_id_zero;
encrypt_parameters* dup_encrypt_parameters(encrypt_parameters* src);
encrypt_parameters* new_encrypt_parameters(uint8_t scheme);
void del_encrypt_parameters(encrypt_parameters* self);

#endif /* __ENCRYPTIONPARAMS_H__ */