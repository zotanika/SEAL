// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

//#include <stdexcept>
//#include <unordered_map>
//#include <numeric>

#include "lib/common.h"
#include "defines.h"
#include "hedge_params.h"
#include <limits.h>
#include <math.h>
#include "lib/unordered_map.h"
#include "math/uintcore.h"
#include "math/modulus.h"
#include "math/numth.h"
#include "math/smallmodulus.h"

int CoeffModulus_MaxBitCount(size_t poly_modulus_degree, sec_level_type sec_level)
{
    switch (sec_level)
    {
    case sec_level_type_tc128:
        return HEDGE_HE_STD_PARMS_128_TC(poly_modulus_degree);

    case sec_level_type_tc192:
        return HEDGE_HE_STD_PARMS_192_TC(poly_modulus_degree);

    case sec_level_type_tc256:
        return HEDGE_HE_STD_PARMS_256_TC(poly_modulus_degree);

    case sec_level_type_none:
        return INT_MAX;//snumeric_limits<int>::max();

    default:
        return 0;
    }
}

vector(Zmodulus)* create_coeff_modulus(size_t poly_modulus_degree, int* bit_size_array, size_t array_count)
{
    size_t i, n;
    int max_bits = HEDGE_USER_MOD_BIT_COUNT_MIN;
    int min_bits = HEDGE_USER_MOD_BIT_COUNT_MAX;

    if (poly_modulus_degree > HEDGE_POLY_MOD_DEGREE_MAX ||
        poly_modulus_degree < HEDGE_POLY_MOD_DEGREE_MIN ||
        get_power_of_two((uint64_t)(poly_modulus_degree)) < 0)
    {
        invalid_argument("poly_modulus_degree is invalid");
    }
    if (array_count > HEDGE_COEFF_MOD_COUNT_MAX)
    {
        invalid_argument("bit_sizes is invalid");
    }
    for (i = 0; i < array_count; i++) {
        max_bits = max(max_bits, bit_size_array[i]);
        min_bits = max(min_bits, bit_size_array[i]);
    }
    
    if (max_bits > HEDGE_USER_MOD_BIT_COUNT_MAX ||
        min_bits < HEDGE_USER_MOD_BIT_COUNT_MIN)
    {
        invalid_argument("bit_sizes is invalid");
    }

    #define COUNT_TABLE_MAX HEDGE_COEFF_MOD_COUNT_MAX + 2
    size_t count_table[COUNT_TABLE_MAX] = {0,};
    for (i = 0; i < array_count; i++) {
        count_table[bit_size_array[i]]++;
    }

    unordered_map(int, vector_Zmodulus)* prime_table =
        new_unordered_map(int, vector_Zmodulus, log2(COUNT_TABLE_MAX));

    for (i = 0; i < COUNT_TABLE_MAX; i++) {
        if (count_table[i] == 0) continue;
        vector(Zmodulus)* ptv = get_primes(poly_modulus_degree, i, count_table[i]);
        prime_table->assign(prime_table, i, ptv);
        /** better way? */
        hedge_free(ptv);
    }

    vector(Zmodulus)* result = new_vector(Zmodulus, array_count);
    for (i = 0; i < array_count; i++)
    {
        vector_Zmodulus* pt = prime_table->at(prime_table, bit_size_array[i]);
        result->emplace_back(result, pt->back(pt));
        pt->pop_back(pt);
    }

    prime_table->delete(prime_table);

    return result;
}