#ifndef __HEDGE_PARAMS_H__
#define __HEDGE_PARAMS_H__

#include "lib/types.h"

int HEDGE_HE_STD_PARMS_128_TC(size_t poly_modulus_degree);
int HEDGE_HE_STD_PARMS_192_TC(size_t poly_modulus_degree);
int HEDGE_HE_STD_PARMS_256_TC(size_t poly_modulus_degree);
int HEDGE_HE_STD_PARMS_128_TQ(size_t poly_modulus_degree);
int HEDGE_HE_STD_PARMS_192_TQ(size_t poly_modulus_degree);
int HEDGE_HE_STD_PARMS_256_TQ(size_t poly_modulus_degree);

// Standard deviation for error distribution
#define HEDGE_HE_STD_PARMS_ERROR_STD_DEV (3.20)

#endif /* __HEDGE_PARAMS_H__ */

