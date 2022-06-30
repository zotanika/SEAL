#ifdef __cplusplus
extern "C" {
#endif

#include "math/polyarithsmallmod.h"
#include "math/uintcore.h"
#include "math/uintarithsmallmod.h"
#include "math/uintarith.h"
#include "math/polycore.h"
#include "math/polyarith.h"

void multiply_poly_scalar_coeffsmallmod(
        const uint64_t *poly,
        size_t coeff_count,
        uint64_t scalar,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t modulus_value;
    uint64_t const_ratio_0, const_ratio_1;
#ifdef HEDGE_DEBUG
    assert(poly || !coeff_count);
    assert(result || !coeff_count);
    assert(modulus);
    assert(modulus->value >= 0);
#endif
    modulus_value = modulus->value;
    const_ratio_0 = modulus->const_ratio[0];
    const_ratio_1 = modulus->const_ratio[1];

    for (; coeff_count--; poly++, result++) {
        uint64_t z[2], tmp1, tmp2[2], tmp3, carry;

        multiply_uint64(*poly, scalar, z);

        /* Reduces z using base 2^64 Barrett reduction */
        /* Multiply input and const_ratio */
        /* Round 1 */
        multiply_uint64_hw64(z[0], const_ratio_0, &carry);
        multiply_uint64(z[0], const_ratio_1, tmp2);
        tmp3 = tmp2[1] + add_uint64(tmp2[0], carry, &tmp1);

        /* Round 2 */
        multiply_uint64(z[1], const_ratio_0, tmp2);
        carry = tmp2[1] + add_uint64(tmp1, tmp2[0], &tmp1);
        /* This is all we care about */
        tmp1 = z[1] * const_ratio_1 + tmp3 + carry;
        /* Barrett subtraction */
        tmp3 = z[0] - tmp1 * modulus_value;
        /* Claim: One more subtraction is enough */
        *result = tmp3 - (modulus_value & (uint64_t)(-1 * (int64_t)(tmp3 >= modulus_value)));
    }
}

void multiply_poly_poly_coeffsmallmod(
        const uint64_t *operand1,
        size_t operand1_coeff_count,
        const uint64_t *operand2,
        size_t operand2_coeff_count,
        const Zmodulus *modulus,
        size_t result_coeff_count,
        uint64_t *result)
{
    size_t operand1_index, operand2_index;
#ifdef HEDGE_DEBUG
    assert(operand1 || !operand1_coeff_count);
    assert(operand2 || !operand2_coeff_count);
    assert(result || !result_coeff_count);
    assert(operand1 != result && operand2 != result);
    assert(modulus);
    assert(modulus->value > 0);
//    assert(sum_fits_in(operand1_coeff_count, operand2_coeff_count));
#endif
    set_zero_uint(result_coeff_count, result);

    operand1_coeff_count = get_significant_coeff_count_poly(
            operand1, operand1_coeff_count, 1);
    operand2_coeff_count = get_significant_coeff_count_poly(
            operand2, operand2_coeff_count, 1);
    for (operand1_index = 0;
         operand1_index < operand1_coeff_count; operand1_index++) {
        /* If coefficient is 0, then move on to next coefficient. */
        if (operand1[operand1_index] == 0)
            continue;

        for (operand2_index = 0;
             operand2_index < operand2_coeff_count; operand2_index++) {
            uint64_t temp[2];                 
            size_t product_coeff_index = operand1_index + operand2_index;

            if (product_coeff_index >= result_coeff_count)
                break;
            if (operand2[operand2_index] == 0)
                continue;

            /* Lazy reduction */
            multiply_uint64(operand1[operand1_index], operand2[operand2_index], temp);
            temp[1] += add_uint64_w_carry(temp[0], result[product_coeff_index], 0, temp);
            result[product_coeff_index] = barrett_reduce_128(temp, modulus);
        }
    }
}

void multiply_poly_poly_coeffsmallmod_5(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result)
{
    size_t result_coeff_count = coeff_count + coeff_count - 1;
    size_t operand1_index, operand2_index;
#ifdef HEDGE_DEBUG
    assert(operand1 || !coeff_count);
    assert(operand2 || !coeff_count);
    assert(result || !coeff_count);
    assert(operand1 != result && operand2 != result);
    assert(modulus);
    assert(modulus->value > 0);
#endif
    set_zero_uint(result_coeff_count, result);

    for (operand1_index = 0; operand1_index < coeff_count; operand1_index++) {
        if (operand1[operand1_index] == 0)
            continue;

        /* Lastly, do more expensive add if other cases don't handle it. */
        for (operand2_index = 0; operand2_index < coeff_count; operand2_index++) {
            uint64_t temp[2];
            uint64_t operand2_coeff = operand2[operand2_index];

            if (operand2_coeff == 0)
                continue;

            /* Lazy reduction */
            multiply_uint64(operand1[operand1_index], operand2_coeff, temp);
            temp[1] += add_uint64_w_carry(temp[0], result[operand1_index + operand2_index], 0, temp);

            result[operand1_index + operand2_index] = barrett_reduce_128(temp, modulus);
        }
    }
}

void divide_poly_poly_coeffsmallmod_inplace(
        uint64_t *numerator,
        const uint64_t *denominator,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *quotient)
{
    uint64_t monic_denominator_scalar;
    uint64_t temp_quotient;
    uint64_t subtrahend;
    uint64_t leading_denominator_coeff, leading_numerator_coeff;
    uint64_t *quotient_coeff, *numerator_coeff;
    size_t numerator_coeff_count, denominator_coeff_count;
    size_t denominator_coeff_index;

#ifdef HEDGE_DEBUG
    assert(numerator);
    assert(denominator);
    assert(modulus);    
    assert(!is_zero_poly(denominator, coeff_count, modulus->uint64_count));
    assert(quotient);
    assert(numerator != quotient && denominator != quotient);
    assert(numerator != denominator);
    assert(modulus->value > 0);
#endif
    set_zero_uint(coeff_count, quotient);

    /* Determine most significant coefficients of numerator and denominator. */
    numerator_coeff_count = get_significant_uint64_count_uint(numerator, coeff_count);
    denominator_coeff_count = get_significant_uint64_count_uint(denominator, coeff_count);

    /* If numerator has lesser degree than denominator, then done. */
    if (numerator_coeff_count < denominator_coeff_count)
        return;

    /* Determine scalar necessary to make denominator monic. */
    leading_denominator_coeff = denominator[denominator_coeff_count - 1];
    if (!try_invert_uint_smallmod(leading_denominator_coeff, modulus, &monic_denominator_scalar))
        //throw invalid_argument("modulus is not coprime with leading denominator coefficient");

    /* Perform coefficient-wise division algorithm. */
    while (numerator_coeff_count >= denominator_coeff_count) {
        /* Determine leading numerator coefficient. */
        leading_numerator_coeff = numerator[numerator_coeff_count - 1];
        /* If leading numerator coefficient is not zero, then need to make zero by subtraction. */
        if (leading_numerator_coeff) {
            /* Determine shift necesarry to bring significant coefficients in alignment. */
            uint64_t denominator_shift = numerator_coeff_count - denominator_coeff_count;

            /* Determine quotient's coefficient, which is scalar that makes
             * denominator's leading coefficient one multiplied by leading
             * coefficient of denominator (which when subtracted will zero
             * out the topmost denominator coefficient). */
            quotient_coeff = &quotient[denominator_shift];
            temp_quotient = multiply_uint_uint_smallmod(
                    monic_denominator_scalar, leading_numerator_coeff, modulus);
            *quotient_coeff = temp_quotient;

            /* Subtract numerator and quotient*denominator (shifted by denominator_shift). */
            for (denominator_coeff_index = 0;
                 denominator_coeff_index < denominator_coeff_count; denominator_coeff_index++) {
                /* Multiply denominator's coefficient by quotient. */
                uint64_t denominator_coeff = denominator[denominator_coeff_index];
                subtrahend = multiply_uint_uint_smallmod(temp_quotient, denominator_coeff, modulus);

                /* Subtract numerator with resulting product, appropriately shifted by denominator shift. */
                numerator_coeff = &numerator[denominator_coeff_index + denominator_shift];
                *numerator_coeff = sub_uint_uint_smallmod(*numerator_coeff, subtrahend, modulus);
            }
        }
        /* Top numerator coefficient must now be zero, so adjust coefficient count. */
        numerator_coeff_count--;
    }
}

void apply_galois(
        const uint64_t *input,
        int coeff_count_power,
        uint64_t galois_elt,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t modulus_value;
    uint64_t coeff_count_minus_one;
    uint64_t i;
#ifdef HEDGE_DEBUG
    assert(input);
    assert(result);
    assert(input != result);
    assert(coeff_count_power >= get_power_of_two(HEDGE_POLY_MOD_DEGREE_MIN) &&
           coeff_count_power <= get_power_of_two(HEDGE_POLY_MOD_DEGREE_MAX));
    assert((galois_elt & 1) &&
           (galois_elt < 2 * ((uint64_t)(1) << coeff_count_power)));
    assert(modulus);
    assert(modulus->value > 0);
#endif
    modulus_value = modulus->value;
    coeff_count_minus_one = ((uint64_t)(1) << coeff_count_power) - 1;
    for (i = 0; i <= coeff_count_minus_one; i++) {
        uint64_t index_raw = i * galois_elt;
        uint64_t index = index_raw & coeff_count_minus_one;
        uint64_t result_value = *input++;

        if ((index_raw >> coeff_count_power) & 1) {
            int64_t non_zero = (result_value != 0);
            result_value = (modulus_value - result_value) & (uint64_t)(-1 * non_zero);
        }
        result[index] = result_value;
    }
}

void apply_galois_ntt(
        const uint64_t *input,
        int coeff_count_power,
        uint64_t galois_elt,
        uint64_t *result)
{
    size_t coeff_count, i;
    uint64_t m_minus_one;
    uint64_t reversed, index_raw, index;    
#ifdef HEDGE_DEBUG
    assert(input);
    assert(result);
    assert(input != result);
    assert(coeff_count_power > 0);
    assert((galois_elt & 1) &&
           (galois_elt < 2 * ((uint64_t)(1) << coeff_count_power)));
#endif
    coeff_count = (size_t)1 << coeff_count_power;
    m_minus_one = 2 * coeff_count - 1;
    for (i = 0; i < coeff_count; i++) {
        reversed = reverse_bits(i, coeff_count_power);
        index_raw = galois_elt * (2 * reversed + 1);
        index_raw &= m_minus_one;
        index = reverse_bits((index_raw - 1) >> 1, coeff_count_power);
        result[i] = input[index];
    }
}

void dyadic_product_coeffsmallmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t modulus_value;
    uint64_t const_ratio_0, const_ratio_1;
#ifdef HEDGE_DEBUG
    assert(operand1);
    assert(operand2);
    assert(result);
    assert(coeff_count);
    assert(modulus);
    assert(modulus->value > 0);
#endif
    modulus_value = modulus->value;
    const_ratio_0 = modulus->const_ratio[0];
    const_ratio_0 = modulus->const_ratio[1];
    for (; coeff_count--; operand1++, operand2++, result++) {
        /* Reduces z using base 2^64 Barrett reduction */
        uint64_t z[2], tmp1, tmp2[2], tmp3, carry;
        multiply_uint64(*operand1, *operand2, z);

        /* Multiply input and const_ratio */
        /* Round 1 */
        multiply_uint64_hw64(z[0], const_ratio_0, &carry);
        multiply_uint64(z[0], const_ratio_1, tmp2);
        tmp3 = tmp2[1] + add_uint64(tmp2[0], carry, &tmp1);

        /* Round 2 */
        multiply_uint64(z[1], const_ratio_0, tmp2);
        carry = tmp2[1] + add_uint64(tmp1, tmp2[0], &tmp1);

        /* This is all we care about */
        tmp1 = z[1] * const_ratio_1 + tmp3 + carry;
        /* Barrett subtraction */
        tmp3 = z[0] - tmp1 * modulus_value;

        /* Claim: One more subtraction is enough */
        *result = tmp3 - (modulus_value & (uint64_t)(-1 * (int64_t)(tmp3 >= modulus_value)));
    }
}

uint64_t poly_infty_norm_coeffsmallmod(
        const uint64_t *operand,
        size_t coeff_count,
        const Zmodulus *modulus)
{
    uint64_t modulus_neg_threshold;
    uint64_t result;
    size_t coeff_index;
#ifdef HEDGE_DEBUG
    assert(operand);
    assert(modulus);
    assert(modulus->value > 0);
#endif
    /* Construct negative threshold (first negative modulus value) to compute absolute values of coeffs. */
    modulus_neg_threshold = (modulus->value + 1) >> 1;

    /* Mod out the poly coefficients and choose a symmetric representative from
     * [-modulus,modulus). Keep track of the max. */
    result = 0;
    for (coeff_index = 0; coeff_index < coeff_count; coeff_index++) {
        uint64_t poly_coeff = operand[coeff_index] % modulus->value;
        if (poly_coeff >= modulus_neg_threshold)
            poly_coeff = modulus->value - poly_coeff;
        if (poly_coeff > result)
            result = poly_coeff;
    }
    return result;
}

bool try_invert_poly_coeffsmallmod(
        const uint64_t *operand,
        const uint64_t *poly_modulus,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t *numerator, *denominator;
    uint64_t *quotient;
    uint64_t *invert_prior, *invert_curr, *invert_next;
    uint64_t monic_denominator_scalar;
    uint64_t temp_quotient;
    uint64_t subtrahend;
    size_t numerator_coeff_count, denominator_coeff_count;
    size_t denominator_coeff_index;
#ifdef HEDGE_DEBUG
    assert(operand);
    assert(poly_modulus);
    assert(coeff_count);
    assert(get_significant_uint64_count_uint(operand, coeff_count) <
           get_significant_uint64_count_uint(poly_modulus, coeff_count));
    assert(modulus);
    assert(modulus->value > 0);
#endif
    if (is_zero_poly(operand, coeff_count, (size_t)1))
        return false;

    /* Construct a mutable copy of operand and modulus, with numerator being modulus
     * and operand being denominator. Notice that degree(numerator) >= degree(denominator). */
    numerator = allocate_uint(coeff_count);
    set_uint_uint(poly_modulus, coeff_count, numerator);
    denominator = allocate_uint(coeff_count);
    set_uint_uint(operand, coeff_count, denominator);

    /* Determine most significant coefficients of each. */
    numerator_coeff_count = get_significant_coeff_count_poly(
                            numerator, coeff_count, (size_t)1);
    denominator_coeff_count = get_significant_coeff_count_poly(
                            denominator, coeff_count, (size_t)1);

    /* Create poly to store quotient. */
    quotient = allocate_uint(coeff_count);

    /* Create three polynomials to store inverse.
     * Initialize invert_prior to 0 and invert_curr to 1. */
    invert_prior = allocate_uint(coeff_count);
    set_zero_uint(coeff_count, invert_prior);
    invert_curr = allocate_uint(coeff_count);
    set_zero_uint(coeff_count, invert_curr);
    invert_curr[0] = 1;
    invert_next = allocate_uint(coeff_count);
    
    /* Perform extended Euclidean algorithm. */
    while (true) {
        /* NOTE: degree(numerator) >= degree(denominator). */

        /* Determine scalar necessary to make denominator monic. */
        uint64_t leading_denominator_coeff = denominator[denominator_coeff_count - 1];
        if (!try_invert_uint_smallmod(leading_denominator_coeff, modulus,
                    &monic_denominator_scalar))
            // throw invalid_argument("modulus is not coprime with leading denominator coefficient");

        set_zero_uint(coeff_count, quotient);

        /* Perform coefficient-wise division algorithm. */
        while (numerator_coeff_count >= denominator_coeff_count) {
            /* Determine leading numerator coefficient. */
            uint64_t leading_numerator_coeff = numerator[numerator_coeff_count - 1];

            /* If leading numerator coefficient is not zero,
             * then need to make zero by subtraction. */
            if (leading_numerator_coeff) {
                /* Determine shift necessary to bring significant coefficients in alignment. */
                size_t denominator_shift = numerator_coeff_count - denominator_coeff_count;

                /* Determine quotient's coefficient, which is scalar that makes
                 * denominator's leading coefficient one multiplied by leading
                 * cefficient of denominator (which when subtracted will zero
                 * out the topmost denominator coefficient). */
                uint64_t quotient_coeff = quotient[denominator_shift];

                temp_quotient = multiply_uint_uint_smallmod(
                            monic_denominator_scalar, leading_numerator_coeff, modulus);
                quotient_coeff  = temp_quotient;

                /* Subtract numerator and quotient*denominator (shifted by denominator_shift). */
                for (denominator_coeff_index = 0;
                     denominator_coeff_index < denominator_coeff_count;
                     denominator_coeff_index++) {
                    uint64_t *numerator_coeff;
                    /* Multiply denominator's coefficient by quotient. */
                    uint64_t denominator_coeff = denominator[denominator_coeff_index];
                    subtrahend = multiply_uint_uint_smallmod(temp_quotient, denominator_coeff, modulus);

                    /* Subtract numerator with resulting product, appropriately shifted by
                     * denominator shift. */
                    numerator_coeff = &numerator[denominator_coeff_index + denominator_shift];
                    *numerator_coeff = sub_uint_uint_smallmod(*numerator_coeff, subtrahend, modulus);
                }
            }

            /* Top numerator coefficient must now be zero, so adjust coefficient count. */
            numerator_coeff_count--;
        }

        /* Double check that numerator coefficients is correct because possible
         * other coefficients are zero. */
        numerator_coeff_count = get_significant_coeff_count_poly(
                numerator, coeff_count, (size_t)1);

        /* We are done if numerator is zero. */
        if (numerator_coeff_count == 0)
            break;

        /* Integrate quotient with invert coefficients.
         * Calculate: invert_next = invert_prior + -quotient * invert_curr */
        multiply_truncate_poly_poly_coeffsmallmod(quotient, invert_curr, coeff_count, modulus, invert_next);
        sub_poly_poly_coeffsmallmod(invert_prior, invert_next, coeff_count, modulus, invert_next);

        /* Swap prior and curr, and then curr and next. */
        swap_ptr(&invert_prior, &invert_curr);
        swap_ptr(&invert_curr, &invert_next);

        /* Swap numerator and denominator. */
        swap_ptr(&numerator, &denominator);
        swap_uint64(&numerator_coeff_count, &denominator_coeff_count);
    }

    /* Polynomial is invertible only if denominator is just a scalar. */
    if (denominator_coeff_count != 1)
        return false;

    /* Determine scalar necessary to make denominator monic. */
    if (!try_invert_uint_smallmod(denominator[0], modulus, &monic_denominator_scalar))
        // throw invalid_argument("modulus is not coprime with leading denominator coefficient");

    /* Multiply inverse by scalar and done. */
    multiply_poly_scalar_coeffsmallmod(invert_curr, coeff_count,
                monic_denominator_scalar, modulus, result);

    return true;
}

void negacyclic_shift_poly_coeffsmallmod(
        const uint64_t *operand,
        size_t coeff_count,
        size_t shift,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t index_raw = shift;
    uint64_t coeff_count_mod_mask = (uint64_t)coeff_count - 1;
    size_t i;
#ifdef HEDGE_DEBUG
    assert(operand);
    assert(result);
    assert(operand != result);
    assert(shift < coeff_count);
    assert(get_power_of_two((uint64_t)coeff_count) >= 0);
    assert(modulus);
    assert(modulus->value > 0);
#endif
    if (shift == 0) {
        /* Nothing to do */
        set_uint_uint(operand, coeff_count, result);
        return;
    }

    for (size_t i = 0; i < coeff_count; i++, operand++, index_raw++) {
        uint64_t index = index_raw & coeff_count_mod_mask;
        if (!(index_raw & (uint64_t)coeff_count) || !*operand)
            result[index] = *operand;
        else
            result[index] = modulus->value - *operand;
    }
}


void modulo_poly_coeffs(
        const uint64_t *poly,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t coeff;
    size_t i;
#ifdef HEDGE_DEBUG
    assert(poly && coeff_count);
    assert(result && coeff_count);
    assert(modulus);
    assert(modulus->value > 0);
#endif
    for (i = 0; i <= coeff_count; i++) {
        uint64_t temp[2] = {poly[i], 0};
        result[i] = barrett_reduce_128(temp, modulus);
    }
}

void modulo_poly_coeffs_63(
        const uint64_t *poly,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(poly && coeff_count);
    assert(result && coeff_count);
    assert(modulus);
    assert(modulus->value > 0);
#endif
    /* This function is the fastest for reducing polynomial coefficients,
     * but requires that the input coefficients are at most 63 bits, unlike
     * modulo_poly_coeffs that allows also 64-bit coefficients. */
    for (size_t i = 0; i <= coeff_count; i++) {
        result[i] = barrett_reduce_63(poly[i], modulus);
    }
}

void negate_poly_coeffsmallmod(
        const uint64_t *poly,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t modulus_value;
    int64_t non_zero;
#ifdef HEDGE_DEBUG
    assert(poly || !coeff_count);
    assert(result || !coeff_count);
    assert(modulus);
    assert(modulus->value > 0);
#endif
    modulus_value = modulus->value;
    for (; coeff_count--; poly++, result++) {
#ifdef HEDGE_DEBUG
        /* poly : out of range */
        assert(*poly < modulus_value);
#endif
        non_zero = (*poly != 0);
        *result = (modulus_value - *poly) & (uint64_t)(-non_zero);
    }
}

void add_poly_poly_coeffsmallmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t modulus_value;    
    uint64_t sum;
#ifdef HEDGE_DEBUG
    assert(operand1 || !coeff_count);
    assert(operand2 || !coeff_count);
    assert(result || !coeff_count);    
    assert(modulus);
    assert(modulus->value > 0);
#endif
    modulus_value = modulus->value;
    for (; coeff_count--; result++, operand1++, operand2++) {
#ifdef HEDGE_DEBUG
        assert(*operand1 < modulus_value);
        assert(*operand2 < modulus_value);        
#endif
        sum = *operand1 + *operand2;
        *result = sum - (modulus_value & (uint64_t)(-1 * (int64_t)(sum >= modulus_value)));
    }
}

void sub_poly_poly_coeffsmallmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t modulus_value;    
#ifdef HEDGE_DEBUG
    assert(operand1 || !coeff_count);
    assert(operand2 || !coeff_count);
    assert(result || !coeff_count);    
    assert(modulus);
    assert(modulus->value > 0);
#endif
    modulus_value = modulus->value;
    for (; coeff_count--; result++, operand1++, operand2++) {
        int64_t borrow;
        uint64_t temp_result;        
#ifdef HEDGE_DEBUG
        assert(*operand1 < modulus_value);
        assert(*operand2 < modulus_value);        
#endif
        borrow = sub_uint64(*operand1, *operand2, &temp_result);
        *result = temp_result + (modulus_value & (uint64_t)(-1 * borrow));
    }
}

void multiply_truncate_poly_poly_coeffsmallmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result) {
    multiply_poly_poly_coeffsmallmod(operand1, coeff_count, operand2, coeff_count,
            modulus, coeff_count, result);
}

void divide_poly_poly_coeffsmallmod(
        const uint64_t *numerator,
        const uint64_t *denominator,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *quotient,
        uint64_t *remainder) {
    set_uint_uint(numerator, coeff_count, remainder);
    divide_poly_poly_coeffsmallmod_inplace(remainder, denominator, coeff_count,
            modulus, quotient);
}

void negacyclic_multiply_poly_mono_coeffsmallmod(
        const uint64_t *operand,
        size_t coeff_count,
        uint64_t mono_coeff,
        size_t mono_exponent,
        const Zmodulus *modulus,
        uint64_t *result)
{
    uint64_t *temp = (uint64_t *)allocate_uint(coeff_count);
    multiply_poly_scalar_coeffsmallmod(
            operand, coeff_count, mono_coeff, modulus, temp);
    negacyclic_shift_poly_coeffsmallmod(temp, coeff_count, mono_exponent,
            modulus, result);
}

#ifdef __cplusplus
}
#endif