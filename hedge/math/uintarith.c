#include "math/uintcore.h"
#include "math/uintarith.h"
#include "lib/common.h"
#include "lib/array.h"
#include "lib/debug.h"

void multiply_uint_uint_6(const uint64_t *operand1,
    size_t operand1_uint64_count, const uint64_t *operand2,
    size_t operand2_uint64_count, size_t result_uint64_count,
    uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand1 && operand1_uint64_count > 0)
    {
        invalid_argument("operand1");
    }
    if (!operand2 && operand2_uint64_count > 0)
    {
        invalid_argument("operand2");
    }
    if (!result_uint64_count)
    {
        invalid_argument("result_uint64_count");
    }
    if (!result)
    {
        invalid_argument("result");
    }
    if (result != nullptr && (operand1 == result || operand2 == result))
    {
        invalid_argument("result cannot point to the same value as operand1 or operand2");
    }
#endif
    // Handle fast cases.
    if (!operand1_uint64_count || !operand2_uint64_count)
    {
        // If either operand is 0, then result is 0.
        set_zero_uint(result_uint64_count, result);
        return;
    }
    if (result_uint64_count == 1)
    {
        *result = *operand1 * *operand2;
        return;
    }

    // In some cases these improve performance.
    operand1_uint64_count = get_significant_uint64_count_uint(
        operand1, operand1_uint64_count);
    operand2_uint64_count = get_significant_uint64_count_uint(
        operand2, operand2_uint64_count);

    // More fast cases
    if (operand1_uint64_count == 1)
    {
        multiply_uint_uint64(operand2, operand2_uint64_count,
            *operand1, result_uint64_count, result);
        return;
    }
    if (operand2_uint64_count == 1)
    {
        multiply_uint_uint64(operand1, operand1_uint64_count,
            *operand2, result_uint64_count, result);
        return;
    }

    // Clear out result.
    set_zero_uint(result_uint64_count, result);

    // Multiply operand1 and operand2.
    size_t operand1_index_max = min(operand1_uint64_count,
        result_uint64_count);
    for (size_t operand1_index = 0;
        operand1_index < operand1_index_max; operand1_index++)
    {
        const uint64_t *inner_operand2 = operand2;
        uint64_t *inner_result = result++;
        uint64_t carry = 0;
        size_t operand2_index = 0;
        size_t operand2_index_max = min(operand2_uint64_count,
            result_uint64_count - operand1_index);
        for (; operand2_index < operand2_index_max; operand2_index++)
        {
            // Perform 64-bit multiplication of operand1 and operand2
            uint64_t temp_result[2];
            multiply_uint64(*operand1, *inner_operand2++, temp_result);
            carry = temp_result[1] + add_uint64_w_carry(temp_result[0], carry, 0, temp_result);
            uint64_t temp;
            carry += add_uint64_w_carry(*inner_result, temp_result[0], 0, &temp);
            *inner_result++ = temp;
        }

        // Write carry if there is room in result
        if (operand1_index + operand2_index_max < result_uint64_count)
        {
            *inner_result = carry;
        }

        operand1++;
    }
}

void multiply_uint_uint64(const uint64_t *operand1,
    size_t operand1_uint64_count, uint64_t operand2,
    size_t result_uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand1 && operand1_uint64_count > 0)
    {
        invalid_argument("operand1");
    }
    if (!result_uint64_count)
    {
        invalid_argument("result_uint64_count");
    }
    if (!result)
    {
        invalid_argument("result");
    }
    if (result != nullptr && operand1 == result)
    {
        invalid_argument("result cannot point to the same value as operand1");
    }
#endif
    // Handle fast cases.
    if (!operand1_uint64_count || !operand2)
    {
        // If either operand is 0, then result is 0.
        set_zero_uint(result_uint64_count, result);
        return;
    }
    if (result_uint64_count == 1)
    {
        *result = *operand1 * operand2;
        return;
    }

    // More fast cases
    //if (result_uint64_count == 2 && operand1_uint64_count > 1)
    //{
    //    uint64_t temp_result;
    //    multiply_uint64(*operand1, operand2, &temp_result);
    //    *result = temp_result;
    //    *(result + 1) += *(operand1 + 1) * operand2;
    //    return;
    //}

    // Clear out result.
    set_zero_uint(result_uint64_count, result);

    // Multiply operand1 and operand2.
    uint64_t carry = 0;
    size_t operand1_index_max = min(operand1_uint64_count,
        result_uint64_count);
    for (size_t operand1_index = 0;
        operand1_index < operand1_index_max; operand1_index++)
    {
        uint64_t temp_result[2];
        multiply_uint64(*operand1++, operand2, temp_result);
        uint64_t temp;
        carry = temp_result[1] + add_uint64_w_carry(temp_result[0], carry, 0, &temp);
        *result++ = temp;
    }

    // Write carry if there is room in result
    if (operand1_index_max < result_uint64_count)
    {
        *result = carry;
    }
}

void divide_uint_uint_inplace(uint64_t *numerator,
    const uint64_t *denominator, size_t uint64_count,
    uint64_t *quotient)
{
#ifdef HEDGE_DEBUG
    if (!numerator && uint64_count > 0)
    {
        invalid_argument("numerator");
    }
    if (!denominator && uint64_count > 0)
    {
        invalid_argument("denominator");
    }
    if (!quotient && uint64_count > 0)
    {
        invalid_argument("quotient");
    }
    if (is_zero_uint(denominator, uint64_count) && uint64_count > 0)
    {
        invalid_argument("denominator");
    }
    if (quotient && (numerator == quotient || denominator == quotient))
    {
        invalid_argument("quotient cannot point to same value as numerator or denominator");
    }
#endif
    if (!uint64_count)
    {
        return;
    }

    // Clear quotient. Set it to zero.
    set_zero_uint(uint64_count, quotient);

    // Determine significant bits in numerator and denominator.
    int numerator_bits =
        get_significant_bit_count_uint(numerator, uint64_count);
    int denominator_bits =
        get_significant_bit_count_uint(denominator, uint64_count);

    // If numerator has fewer bits than denominator, then done.
    if (numerator_bits < denominator_bits)
    {
        return;
    }

    // Only perform computation up to last non-zero uint64s.
    uint64_count = (size_t)(
        divide_round_up(numerator_bits, BITS_PER_UINT64));

    // Handle fast case.
    if (uint64_count == 1)
    {
        *quotient = *numerator / *denominator;
        *numerator -= *quotient * *denominator;
        return;
    }

    uint64_t* alloc_anchor = allocate_uint(uint64_count << 1);

    // Create temporary space to store mutable copy of denominator.
    uint64_t *shifted_denominator = alloc_anchor;

    // Create temporary space to store difference calculation.
    uint64_t *difference = shifted_denominator + uint64_count;

    // Shift denominator to bring MSB in alignment with MSB of numerator.
    int denominator_shift = numerator_bits - denominator_bits;
    left_shift_uint(denominator, denominator_shift, uint64_count,
        shifted_denominator);
    denominator_bits += denominator_shift;

    // Perform bit-wise division algorithm.
    int remaining_shifts = denominator_shift;
    while (numerator_bits == denominator_bits)
    {
        // NOTE: MSBs of numerator and denominator are aligned.

        // Even though MSB of numerator and denominator are aligned,
        // still possible numerator < shifted_denominator.
        if (sub_uint_uint(numerator, shifted_denominator,
            uint64_count, difference))
        {
            // numerator < shifted_denominator and MSBs are aligned,
            // so current quotient bit is zero and next one is definitely one.
            if (remaining_shifts == 0)
            {
                // No shifts remain and numerator < denominator so done.
                break;
            }

            // Effectively shift numerator left by 1 by instead adding
            // numerator to difference (to prevent overflow in numerator).
            add_uint_uint(difference, numerator, uint64_count, difference);

            // Adjust quotient and remaining shifts as a result of
            // shifting numerator.
            left_shift_uint(quotient, 1, uint64_count, quotient);
            remaining_shifts--;
        }
        // Difference is the new numerator with denominator subtracted.

        // Update quotient to reflect subtraction.
        quotient[0] |= 1;

        // Determine amount to shift numerator to bring MSB in alignment
        // with denominator.
        numerator_bits = get_significant_bit_count_uint(difference, uint64_count);
        int numerator_shift = denominator_bits - numerator_bits;
        if (numerator_shift > remaining_shifts)
        {
            // Clip the maximum shift to determine only the integer
            // (as opposed to fractional) bits.
            numerator_shift = remaining_shifts;
        }

        // Shift and update numerator.
        if (numerator_bits > 0)
        {
            left_shift_uint(difference, numerator_shift, uint64_count, numerator);
            numerator_bits += numerator_shift;
        }
        else
        {
            // Difference is zero so no need to shift, just set to zero.
            set_zero_uint(uint64_count, numerator);
        }

        // Adjust quotient and remaining shifts as a result of shifting numerator.
        left_shift_uint(quotient, numerator_shift, uint64_count, quotient);
        remaining_shifts -= numerator_shift;
    }

    // Correct numerator (which is also the remainder) for shifting of
    // denominator, unless it is just zero.
    if (numerator_bits > 0)
    {
        right_shift_uint(numerator, denominator_shift, uint64_count, numerator);
    }
}

void divide_uint128_uint64_inplace_generic(uint64_t *numerator,
    uint64_t denominator, uint64_t *quotient)
{
#ifdef HEDGE_DEBUG
    if (!numerator)
    {
        invalid_argument("numerator");
    }
    if (denominator == 0)
    {
        invalid_argument("denominator");
    }
    if (!quotient)
    {
        invalid_argument("quotient");
    }
    if (numerator == quotient)
    {
        invalid_argument("quotient cannot point to same value as numerator");
    }
#endif
    // We expect 129-bit input
    const size_t uint64_count = 2;

    // Clear quotient. Set it to zero.
    quotient[0] = 0;
    quotient[1] = 0;

    // Determine significant bits in numerator and denominator.
    int numerator_bits = get_significant_bit_count_uint(numerator, uint64_count);
    int denominator_bits = get_significant_bit_count(denominator);

    // If numerator has fewer bits than denominator, then done.
    if (numerator_bits < denominator_bits)
    {
        return;
    }

    // Create temporary space to store mutable copy of denominator.
    uint64_t shifted_denominator[2] = { denominator, 0 };

    // Create temporary space to store difference calculation.
    uint64_t difference[2] = { 0, 0 };

    // Shift denominator to bring MSB in alignment with MSB of numerator.
    int denominator_shift = numerator_bits - denominator_bits;

    left_shift_uint128(shifted_denominator, denominator_shift, shifted_denominator);
    denominator_bits += denominator_shift;

    // Perform bit-wise division algorithm.
    int remaining_shifts = denominator_shift;
    while (numerator_bits == denominator_bits)
    {
        // NOTE: MSBs of numerator and denominator are aligned.

        // Even though MSB of numerator and denominator are aligned,
        // still possible numerator < shifted_denominator.
        if (sub_uint_uint(numerator, shifted_denominator, uint64_count, difference))
        {
            // numerator < shifted_denominator and MSBs are aligned,
            // so current quotient bit is zero and next one is definitely one.
            if (remaining_shifts == 0)
            {
                // No shifts remain and numerator < denominator so done.
                break;
            }

            // Effectively shift numerator left by 1 by instead adding
            // numerator to difference (to prevent overflow in numerator).
            add_uint_uint(difference, numerator, uint64_count, difference);

            // Adjust quotient and remaining shifts as a result of shifting numerator.
            quotient[1] = (quotient[1] << 1) | (quotient[0] >> (BITS_PER_UINT64 - 1));
            quotient[0] <<= 1;
            remaining_shifts--;
        }
        // Difference is the new numerator with denominator subtracted.

        // Determine amount to shift numerator to bring MSB in alignment
        // with denominator.
        numerator_bits = get_significant_bit_count_uint(difference, uint64_count);

        // Clip the maximum shift to determine only the integer
        // (as opposed to fractional) bits.
        int numerator_shift = min(denominator_bits - numerator_bits, remaining_shifts);

        // Shift and update numerator.
        // This may be faster; first set to zero and then update if needed

        // Difference is zero so no need to shift, just set to zero.
        numerator[0] = 0;
        numerator[1] = 0;

        if (numerator_bits > 0)
        {
            left_shift_uint128(difference, numerator_shift, numerator);
            numerator_bits += numerator_shift;
        }

        // Update quotient to reflect subtraction.
        quotient[0] |= 1;

        // Adjust quotient and remaining shifts as a result of shifting numerator.
        left_shift_uint128(quotient, numerator_shift, quotient);
        remaining_shifts -= numerator_shift;
    }

    // Correct numerator (which is also the remainder) for shifting of
    // denominator, unless it is just zero.
    if (numerator_bits > 0)
    {
        right_shift_uint128(numerator, denominator_shift, numerator);
    }
}

void divide_uint192_uint64_inplace(uint64_t *numerator,
    uint64_t denominator, uint64_t *quotient)
{
#ifdef HEDGE_DEBUG
    if (!numerator)
    {
        invalid_argument("numerator");
    }
    if (denominator == 0)
    {
        invalid_argument("denominator");
    }
    if (!quotient)
    {
        invalid_argument("quotient");
    }
    if (numerator == quotient)
    {
        invalid_argument("quotient cannot point to same value as numerator");
    }
#endif
    // We expect 192-bit input
    size_t uint64_count = 3;

    // Clear quotient. Set it to zero.
    quotient[0] = 0;
    quotient[1] = 0;
    quotient[2] = 0;

    // Determine significant bits in numerator and denominator.
    int numerator_bits = get_significant_bit_count_uint(numerator, uint64_count);
    int denominator_bits = get_significant_bit_count(denominator);

    log_verbose_extreme("numerator_bits=%d\n", numerator_bits);
    log_verbose_extreme("denominator_bits=%d\n", denominator_bits);
    // If numerator has fewer bits than denominator, then done.
    if (numerator_bits < denominator_bits)
    {
        return;
    }

    // Only perform computation up to last non-zero uint64s.
    uint64_count = (size_t)(
        divide_round_up(numerator_bits, BITS_PER_UINT64));

    log_verbose("numerator_bits=%d denominator_bits=%d bits_per_uint64=%lu uint64_count=%lu\n", 
        numerator_bits, denominator_bits, BITS_PER_UINT64, uint64_count);
    // Handle fast case.
    if (uint64_count == 1)
    {
        *quotient = *numerator / denominator;
        *numerator -= *quotient * denominator;
        return;
    }

    // Create temporary space to store mutable copy of denominator.
    //uint64_t* shifted_denominator = zallocate_uint(uint64_count);
    uint64_t shifted_denominator[3] = {0, };
    shifted_denominator[0] = denominator;

    // Create temporary space to store difference calculation.
    //uint64_t* difference = zallocate_uint(uint64_count);
    uint64_t difference[3] = {0, };

    // Shift denominator to bring MSB in alignment with MSB of numerator.
    int denominator_shift = numerator_bits - denominator_bits;

    left_shift_uint192(shifted_denominator, denominator_shift,
        shifted_denominator);
    denominator_bits += denominator_shift;

#ifdef HEDGE_DEBUG
    for (size_t i = 0; i < uint64_count; i++) {
        log_verbose("shifted_denominator[%d]=%lu difference[%d]=%lu\n",
            i, shifted_denominator[i], i, difference[i]);
    }
    log_verbose("denominator_bits=%lu denominator_shift=%lu\n",
        denominator_bits, denominator_shift);
#endif

    // Perform bit-wise division algorithm.
    int remaining_shifts = denominator_shift;
    while (numerator_bits == denominator_bits)
    {
        // NOTE: MSBs of numerator and denominator are aligned.

        // Even though MSB of numerator and denominator are aligned,
        // still possible numerator < shifted_denominator.
        if (sub_uint_uint(numerator, shifted_denominator,
            uint64_count, difference))
        {
            // numerator < shifted_denominator and MSBs are aligned,
            // so current quotient bit is zero and next one is definitely one.
            if (remaining_shifts == 0)
            {
                // No shifts remain and numerator < denominator so done.
                break;
            }
            log_verbose("sub result (%lu, %lu) difference = {%lu, %lu, %lu}\n",
                numerator_bits, denominator_bits,
                difference[0], difference[1], difference[2]);

            // Effectively shift numerator left by 1 by instead adding
            // numerator to difference (to prevent overflow in numerator).
            add_uint_uint(difference, numerator, uint64_count, difference);
            log_verbose("add result (%lu, %lu) difference = {%lu, %lu, %lu}\n",
                numerator_bits, denominator_bits,
                difference[0], difference[1], difference[2]);
            // Adjust quotient and remaining shifts as a result of shifting numerator.
            left_shift_uint192(quotient, 1, quotient);
            remaining_shifts--;
        }
        // Difference is the new numerator with denominator subtracted.
        log_verbose("-> sub result (%lu, %lu) difference = {%lu, %lu, %lu}\n",
            numerator_bits, denominator_bits,
            difference[0], difference[1], difference[2]);
        // Update quotient to reflect subtraction.
        quotient[0] |= 1;

        // Determine amount to shift numerator to bring MSB in alignment with denominator.
        numerator_bits = get_significant_bit_count_uint(difference, uint64_count);
        int numerator_shift = denominator_bits - numerator_bits;
        if (numerator_shift > remaining_shifts)
        {
            // Clip the maximum shift to determine only the integer
            // (as opposed to fractional) bits.
            numerator_shift = remaining_shifts;
        }

        // Shift and update numerator.
        if (numerator_bits > 0)
        {
            left_shift_uint192(difference, numerator_shift, numerator);
            numerator_bits += numerator_shift;
        }
        else
        {
            // Difference is zero so no need to shift, just set to zero.
            set_zero_uint(uint64_count, numerator);
        }

        // Adjust quotient and remaining shifts as a result of shifting numerator.
        left_shift_uint192(quotient, numerator_shift, quotient);
        remaining_shifts -= numerator_shift;
    }

    // Correct numerator (which is also the remainder) for shifting of
    // denominator, unless it is just zero.
    if (numerator_bits > 0)
    {
        right_shift_uint192(numerator, denominator_shift, numerator);
    }
    //hedge_free(shifted_denominator);
    //hedge_free(difference);
}

void exponentiate_uint(const uint64_t *operand,
    size_t operand_uint64_count, const uint64_t *exponent,
    size_t exponent_uint64_count, size_t result_uint64_count,
    uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand)
    {
        invalid_argument("operand");
    }
    if (!operand_uint64_count)
    {
        invalid_argument("operand_uint64_count");
    }
    if (!exponent)
    {
        invalid_argument("exponent");
    }
    if (!exponent_uint64_count)
    {
        invalid_argument("exponent_uint64_count");
    }
    if (!result)
    {
        invalid_argument("result");
    }
    if (!result_uint64_count)
    {
        invalid_argument("result_uint64_count");
    }
#endif
    // Fast cases
    if (is_zero_uint(exponent, exponent_uint64_count))
    {
        set_uint(1, result_uint64_count, result);
        return;
    }
    if (is_equal_uint(exponent, exponent_uint64_count, 1))
    {
        set_uint_uint_4(operand, operand_uint64_count, result_uint64_count, result);
        return;
    }

    // Need to make a copy of exponent
    uint64_t* exponent_copy = allocate_uint(exponent_uint64_count);
    set_uint_uint(exponent, exponent_uint64_count, exponent_copy);

    // Perform binary exponentiation.
    uint64_t* big_alloc = allocate_uint(
        result_uint64_count + result_uint64_count + result_uint64_count);

    uint64_t *powerptr = big_alloc;
    uint64_t *productptr = powerptr + result_uint64_count;
    uint64_t *intermediateptr = productptr + result_uint64_count;

    set_uint_uint_4(operand, operand_uint64_count, result_uint64_count, powerptr);
    set_uint(1, result_uint64_count, intermediateptr);

    // Initially: power = operand and intermediate = 1, product is not initialized.
    while (true)
    {
        if ((*exponent_copy % 2) == 1)
        {
            multiply_truncate_uint_uint(powerptr, intermediateptr,
                result_uint64_count, productptr);
            swap_ptr(&productptr, &intermediateptr);
        }
        right_shift_uint(exponent_copy, 1, exponent_uint64_count,
            exponent_copy);
        if (is_zero_uint(exponent_copy, exponent_uint64_count))
        {
            break;
        }
        multiply_truncate_uint_uint(powerptr, powerptr, result_uint64_count,
            productptr);
        swap_ptr(&productptr, &powerptr);
    }
    set_uint_uint(intermediateptr, result_uint64_count, result);
}

uint64_t exponentiate_uint64_safe(uint64_t operand, uint64_t exponent)
{
    // Fast cases
    if (exponent == 0)
    {
        return 1;
    }
    if (exponent == 1)
    {
        return operand;
    }

    // Perform binary exponentiation.
    uint64_t power = operand;
    uint64_t product = 0;
    uint64_t intermediate = 1;

    // Initially: power = operand and intermediate = 1, product irrelevant.
    while (true)
    {
        if (exponent & 1)
        {
            product = mul_safe(power, intermediate);
            swap_uint64(&product, &intermediate);
        }
        exponent >>= 1;
        if (exponent == 0)
        {
            break;
        }
        product = mul_safe(power, power);
        swap_uint64(&product, &power);
    }

    return intermediate;
}

uint64_t exponentiate_uint64(uint64_t operand, uint64_t exponent)
{
    // Fast cases
    if (exponent == 0)
    {
        return 1;
    }
    if (exponent == 1)
    {
        return operand;
    }

    // Perform binary exponentiation.
    uint64_t power = operand;
    uint64_t product = 0;
    uint64_t intermediate = 1;

    // Initially: power = operand and intermediate = 1, product irrelevant.
    while (true)
    {
        if (exponent & 1)
        {
            product = power * intermediate;
            swap_uint64(&product, &intermediate);
        }
        exponent >>= 1;
        if (exponent == 0)
        {
            break;
        }
        product = power * power;
        swap_uint64(&product, &power);
    }

    return intermediate;
}

