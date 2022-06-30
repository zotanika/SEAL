#ifdef __cplusplus
extern "C" {
#endif

#include "uintarithmod.h"

bool try_invert_uint_mod(
        const uint64_t *operand,
        const uint64_t *modulus,
        size_t uint64_count,
        uint64_t *result)
{
    int bit_count;
    void *alloc_anchor;
    uint64_t *numerator;
    uint64_t *denominator;
    uint64_t *difference;
    uint64_t *quotient;
    uint64_t *invert_prior;
    uint64_t *invert_curr;
    uint64_t *invert_next;
    int numerator_bits;
    int denominator_bits;
    bool invert_prior_positive;
    bool invert_curr_positive;
    bool invert_next_positive;

#ifdef HEDGE_DEBUG
    assert(operand);
    assert(modulus);
    assert(uint64_count);
    assert(result);
    assert(is_less_than_uint_uint(operand, modulus, uint64_count));
#endif
    /* Cannot invert 0. */
    bit_count = get_significant_bit_count_uint(operand, uint64_count);
    if (bit_count == 0)
        return false;

    /* If it is 1, then its invert is itself. */
    if (bit_count == 1) {
        set_uint(1, uint64_count, result);
        return true;
    }

    alloc_anchor = allocate_uint(7 * uint64_count);
    /* Construct a mutable copy of operand and modulus, with numerator being modulus
     * and operand being denominator. Notice that numerator > denominator. */
    numerator = (uint64_t *)alloc_anchor;
    set_uint_uint(modulus, uint64_count, numerator);

    denominator = numerator + uint64_count;
    set_uint_uint(operand, uint64_count, denominator);

    /* Create space to store difference. */
    difference = denominator + uint64_count;

    /* Determine highest bit index of each. */
    numerator_bits = get_significant_bit_count_uint(numerator, uint64_count);
    denominator_bits = get_significant_bit_count_uint(denominator, uint64_count);

    /* Create space to store quotient. */
    quotient = difference + uint64_count;

    /* Create three sign/magnitude values to store coefficients.
     * Initialize invert_prior to +0 and invert_curr to +1. */
    invert_prior = quotient + uint64_count;
    set_zero_uint(uint64_count, invert_prior);
    invert_prior_positive = true;

    invert_curr = invert_prior + uint64_count;
    set_uint(1, uint64_count, invert_curr);
    invert_curr_positive = true;

    invert_next = invert_curr + uint64_count;
    invert_next_positive = true;

    /* Perform extended Euclidean algorithm. */
    while (true) {
        size_t division_uint64_count;
        int numerator_shift;
        int denominator_shift;
        int remaining_shifts;

        /* NOTE: Numerator is > denominator. */

        /* Only perform computation up to last non-zero uint64s. */
        division_uint64_count = (size_t)divide_round_up(numerator_bits, BITS_PER_UINT64);
        /* Shift denominator to bring MSB in alignment with MSB of numerator. */
        denominator_shift = numerator_bits - denominator_bits;

        left_shift_uint(denominator, denominator_shift, division_uint64_count, denominator);
        denominator_bits += denominator_shift;

        /* Clear quotient. */
        set_zero_uint(uint64_count, quotient);

        /* Perform bit-wise division algorithm. */
        remaining_shifts = denominator_shift;
        while (numerator_bits == denominator_bits) {
            /* NOTE: MSBs of numerator and denominator are aligned. */

            /* Even though MSB of numerator and denominator are aligned,
             * still possible numerator < denominator. */
            if (sub_uint_uint(numerator, denominator,
                division_uint64_count, difference)) {
                /* numerator < denominator and MSBs are aligned, so current
                 * quotient bit is zero and next one is definitely one.*/
                if (remaining_shifts == 0) {
                    /* No shifts remain and numerator < denominator so done. */
                    break;
                }

                /* Effectively shift numerator left by 1 by instead adding
                 * numerator to difference (to prevent overflow in numerator). */
                add_uint_uint(difference, numerator, division_uint64_count, difference);

                /* Adjust quotient and remaining shifts as a result of shifting numerator. */
                left_shift_uint(quotient, 1, division_uint64_count, quotient);
                remaining_shifts--;
            }

            /* Difference is the new numerator with denominator subtracted. */

            /* Update quotient to reflect subtraction. */
            *quotient |= 1;

            /* Determine amount to shift numerator to bring MSB in alignment
             * with denominator. */
            numerator_bits
                    = get_significant_bit_count_uint(difference, division_uint64_count);
            numerator_shift = denominator_bits - numerator_bits;
            if (numerator_shift > remaining_shifts) {
                /* Clip the maximum shift to determine only the integer
                 * (as opposed to fractional) bits. */
                numerator_shift = remaining_shifts;
            }

            /* Shift and update numerator. */
            if (numerator_bits > 0) {
                left_shift_uint(difference, numerator_shift, division_uint64_count, numerator);
                numerator_bits += numerator_shift;
            } else {
                /* Difference is zero so no need to shift, just set to zero. */
                set_zero_uint(division_uint64_count, numerator);
            }

            /* Adjust quotient and remaining shifts as a result of shifting numerator. */
            left_shift_uint(quotient, numerator_shift, division_uint64_count, quotient);
            remaining_shifts -= numerator_shift;
        }

        /* Correct for shifting of denominator. */
        right_shift_uint(denominator, denominator_shift, division_uint64_count, denominator);
        denominator_bits -= denominator_shift;

        /* We are done if remainder (which is stored in numerator) is zero. */
        if (numerator_bits == 0)
            break;

        /* Correct for shifting of denominator. */
        right_shift_uint(numerator, denominator_shift, division_uint64_count, numerator);
        numerator_bits -= denominator_shift;

        /* Integrate quotient with invert coefficients.
         * Calculate: invert_prior + -quotient * invert_curr */
        multiply_truncate_uint_uint(quotient, invert_curr, uint64_count, invert_next);
        invert_next_positive = !invert_curr_positive;
        if (invert_prior_positive == invert_next_positive) {
            /* If both sides of add have same sign, then simple add and
             * do not need to worry about overflow due to known limits
             * on the coefficients proved in the euclidean algorithm. */
            add_uint_uint(invert_prior, invert_next, uint64_count, invert_next);
        } else {
            /* If both sides of add have opposite sign, then subtract
             * and check for overflow. */
            uint64_t borrow = sub_uint_uint(invert_prior, invert_next, uint64_count, invert_next);
            if (borrow == 0) {
                /* No borrow means |invert_prior| >= |invert_next|,
                 * so sign is same as invert_prior. */
                invert_next_positive = invert_prior_positive;
            } else {
                /* Borrow means |invert prior| < |invert_next|,
                 * so sign is opposite of invert_prior. */
                invert_next_positive = !invert_prior_positive;
                negate_uint(invert_next, uint64_count, invert_next);
            }
        }

        /* Swap prior and curr, and then curr and next. */
        swap_ptr(&invert_prior, &invert_curr);
        swap_bool(&invert_prior_positive, &invert_curr_positive);
        swap_ptr(&invert_curr, &invert_next);
        swap_bool(&invert_curr_positive, &invert_next_positive);

        /* Swap numerator and denominator using pointer swings. */
        swap_ptr(&numerator, &denominator);
        swap_int(&numerator_bits, &denominator_bits);
    }

    if (!is_equal_uint(denominator, uint64_count, 1)) {
        /* GCD is not one, so unable to find inverse. */
        return false;
    }

    /* Correct coefficient if negative by modulo. */
    if (!invert_curr_positive && !is_zero_uint(invert_curr, uint64_count)) {
        sub_uint_uint(modulus, invert_curr, uint64_count, invert_curr);
        invert_curr_positive = true;
    }

    /* Set result. */
    set_uint_uint(invert_curr, uint64_count, result);
    return true;
}

#ifdef __cplusplus
}
#endif