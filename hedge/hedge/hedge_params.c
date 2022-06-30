
#include "hedge_params.h"

/**
Largest allowed bit counts for coeff_modulus based on the security estimates from
HomomorphicEncryption.org security standard. Microsoft SEAL samples the secret key
from a ternary {-1, 0, 1} distribution.
*/
// Ternary secret; 128 bits classical security
int HEDGE_HE_STD_PARMS_128_TC(size_t poly_modulus_degree)
{
    switch (poly_modulus_degree)
    {
        case (size_t)(1024):      return 27;
        case (size_t)(2048):      return 54;
        case (size_t)(4096):      return 109;
        case (size_t)(8192):      return 218;
        case (size_t)(16384):     return 438;
        case (size_t)(32768):     return 881;
    }
    return 0;
}

// Ternary secret; 192 bits classical security
int HEDGE_HE_STD_PARMS_192_TC(size_t poly_modulus_degree)
{
    switch (poly_modulus_degree)
    {
        case (size_t)(1024):      return 19;
        case (size_t)(2048):      return 37;
        case (size_t)(4096):      return 75;
        case (size_t)(8192):      return 152;
        case (size_t)(16384):     return 305;
        case (size_t)(32768):     return 611;
    }
    return 0;
}

// Ternary secret; 256 bits classical security
int HEDGE_HE_STD_PARMS_256_TC(size_t poly_modulus_degree)
{
    switch (poly_modulus_degree)
    {
        case (size_t)(1024):      return 14;
        case (size_t)(2048):      return 29;
        case (size_t)(4096):      return 58;
        case (size_t)(8192):      return 118;
        case (size_t)(16384):     return 237;
        case (size_t)(32768):     return 476;
    }
    return 0;
}

// Ternary secret; 128 bits quantum security
int HEDGE_HE_STD_PARMS_128_TQ(size_t poly_modulus_degree)
{
    switch (poly_modulus_degree)
    {
        case (size_t)(1024):      return 25;
        case (size_t)(2048):      return 51;
        case (size_t)(4096):      return 101;
        case (size_t)(8192):      return 202;
        case (size_t)(16384):     return 411;
        case (size_t)(32768):     return 827;
    }
    return 0;
}

// Ternary secret; 192 bits quantum security
int HEDGE_HE_STD_PARMS_192_TQ(size_t poly_modulus_degree)
{
    switch (poly_modulus_degree)
    {
        case (size_t)(1024):      return 17;
        case (size_t)(2048):      return 35;
        case (size_t)(4096):      return 70;
        case (size_t)(8192):      return 141;
        case (size_t)(16384):     return 284;
        case (size_t)(32768):     return 571;
    }
    return 0;
}

// Ternary secret; 256 bits quantum security
int HEDGE_HE_STD_PARMS_256_TQ(size_t poly_modulus_degree)
{
    switch (poly_modulus_degree)
    {
        case (size_t)(1024):      return 13;
        case (size_t)(2048):      return 27;
        case (size_t)(4096):      return 54;
        case (size_t)(8192):      return 109;
        case (size_t)(16384):     return 220;
        case (size_t)(32768):     return 443;
    }
    return 0;
}
