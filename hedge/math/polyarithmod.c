#include "math/polyarithmod.h"

#ifdef __cplusplus
extern "C" {
#endif

void poly_infty_norm_coeffmod(
        const uint64_t *poly,
        size_t coeff_count,
        size_t coeff_uint64_count,
        const uint64_t *modulus,
        uint64_t *result)
{
    /* Construct negative threshold (first negative modulus value) to compute
     * absolute values of coeffs. */
    uint64_t *modulus_neg_threshold = allocate_uint(coeff_uint64_count);
    uint64_t *coeff_abs_value = allocate_uint(coeff_uint64_count);
    size_t i;

    /* Set to value of (modulus + 1) / 2. To prevent overflowing with the +1, just
     * add 1 to the result if modulus was odd. */
    half_round_up_uint(modulus, coeff_uint64_count, modulus_neg_threshold);

    /* Mod out the poly coefficients and choose a symmetric representative from
     * [-modulus,modulus). Keep track of the max. */
    set_zero_uint(coeff_uint64_count, result);
    
    for (i = 0; i < coeff_count; i++, poly += coeff_uint64_count) {
        if (is_greater_than_or_equal_uint_uint(
                poly, modulus_neg_threshold, coeff_uint64_count)) {
            sub_uint_uint(modulus, poly, coeff_uint64_count, coeff_abs_value);
        } else {
            set_uint_uint(poly, coeff_uint64_count, coeff_abs_value);
        }

        if (is_greater_than_uint_uint(coeff_abs_value, result, coeff_uint64_count)) {
            set_uint_uint(coeff_abs_value, coeff_uint64_count, result);
        }
    }
}

#ifdef __cplusplus
}
#endif