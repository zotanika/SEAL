#ifndef __MODULUS_H__
#define __MODULUS_H__

#include "lib/types.h"
#include "math/smallmodulus.h"
#include "lib/vector.h"

typedef enum 
{
    /**
    No security level specified.
    */
    sec_level_type_none = 0,

    /**
    128-bit security level according to HomomorphicEncryption.org standard.
    */
    sec_level_type_tc128 = 128,

    /**
    192-bit security level according to HomomorphicEncryption.org standard.
    */
    sec_level_type_tc192 = 192,

    /**
    256-bit security level according to HomomorphicEncryption.org standard.
    */
    sec_level_type_tc256 = 256
} sec_level_type;

// CoeffModulus has static functions only... so it's functions are implemented
// in C file without classes
int CoeffModulus_MaxBitCount(size_t poly_modulus_degree, sec_level_type sec_level);

vector(Zmodulus)* create_coeff_modulus(size_t poly_modulus_degree, int* bit_size_array, size_t array_count);

// PlainModulus only used in BFVDefault

#endif /* __MODULUS_H__ */
