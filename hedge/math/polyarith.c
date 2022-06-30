#include "math/polyarith.h"
#include "lib/common.h"

#ifdef __cplusplus
extern "C" {
#endif

void multiply_poly_poly(
            const uint64_t *operand1,
            size_t operand1_coeff_count,
            size_t operand1_coeff_uint64_count,
            const uint64_t *operand2,
            size_t operand2_coeff_count,
            size_t operand2_coeff_uint64_count,
            size_t result_coeff_count,
            size_t result_coeff_uint64_count,
            uint64_t *result)
{
    uint64_t *intermediate = allocate_uint(result_coeff_uint64_count);
    size_t operand1_index, operand2_index;
    uint64_t *operand1_coeff;
    uint64_t *operand2_coeff;
    uint64_t *result_coeff;

    set_zero_poly(result_coeff_count, result_coeff_uint64_count, result);

    operand1_coeff_count = get_significant_coeff_count_poly(
        operand1, operand1_coeff_count, operand1_coeff_uint64_count);
    operand2_coeff_count = get_significant_coeff_count_poly(
        operand2, operand2_coeff_count, operand2_coeff_uint64_count);

    for (operand1_index = 0;
         operand1_index < operand1_coeff_count; operand1_index++) {
        operand1_coeff = (uint64_t *)get_poly_coeff(
                operand1, operand1_index, operand1_coeff_uint64_count);

        for (operand2_index = 0;
             operand2_index < operand2_coeff_count; operand2_index++) {
            size_t product_coeff_index = operand1_index + operand2_index;

            if (product_coeff_index >= result_coeff_count)
                break;

            operand2_coeff = (uint64_t *)get_poly_coeff(
                    (const uint64_t *)operand2, operand2_index, operand2_coeff_uint64_count);
            multiply_uint_uint_6((const uint64_t *)operand1_coeff, operand1_coeff_uint64_count,
                    operand2_coeff, operand2_coeff_uint64_count,
                    result_coeff_uint64_count, intermediate);
            result_coeff = (uint64_t *)get_poly_coeff(
                    (const uint64_t *)result, product_coeff_index, result_coeff_uint64_count);
            add_uint_uint((const uint64_t *)result_coeff, (const uint64_t *)intermediate,
                    result_coeff_uint64_count, result_coeff);
        }
    }
}

void poly_eval_poly(
            const uint64_t *poly_to_eval,
            size_t poly_to_eval_coeff_count,
            size_t poly_to_eval_coeff_uint64_count,
            const uint64_t *value,
            size_t value_coeff_count,
            size_t value_coeff_uint64_count,
            size_t result_coeff_count,
            size_t result_coeff_uint64_count,
            uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(poly_to_eval);
    assert(value);
    assert(result);
    assert(poly_to_eval_coeff_count && poly_to_eval_coeff_uint64_count && value_coeff_count ||
           value_coeff_uint64_count && result_coeff_count && result_coeff_uint64_count);
#endif
    /* Evaluate poly at value using Horner's method */
    uint64_t *productptr = allocate_poly(result_coeff_count, result_coeff_uint64_count);
    uint64_t *intermediateptr = allocate_zero_poly(result_coeff_count, result_coeff_uint64_count);

    while (poly_to_eval_coeff_count--) {
        multiply_poly_poly(intermediateptr, result_coeff_count,
                result_coeff_uint64_count, value, value_coeff_count,
                value_coeff_uint64_count, result_coeff_count,
                result_coeff_uint64_count, productptr);

        const uint64_t *curr_coeff = get_poly_coeff(
                poly_to_eval, poly_to_eval_coeff_count,
                poly_to_eval_coeff_uint64_count);
        add_uint_uint_w_carry((const uint64_t *)productptr, result_coeff_uint64_count, curr_coeff,
                poly_to_eval_coeff_uint64_count, false,
                result_coeff_uint64_count, productptr);
        swap_ptr(&productptr, &intermediateptr);
    }

    set_poly_poly_simple((const uint64_t *)intermediateptr, result_coeff_count,
            result_coeff_uint64_count, result);
}

void exponentiate_poly(
            const uint64_t *poly,
            size_t poly_coeff_count,
            size_t poly_coeff_uint64_count,
            const uint64_t *exponent,
            size_t exponent_uint64_count,
            size_t result_coeff_count,
            size_t result_coeff_uint64_count,
            uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(poly);
    assert(exponent);
    assert(result);
    assert(poly_coeff_count && poly_coeff_uint64_count && exponent_uint64_count &&
           result_coeff_count && result_coeff_uint64_count);
#endif
    uint64_t *exponent_copy = allocate_uint(exponent_uint64_count);
    uint64_t *powerptr = allocate_uint(mul_safe(
                add_safe(result_coeff_count, result_coeff_count, result_coeff_count),
                result_coeff_uint64_count));
    uint64_t *productptr;
    uint64_t *intermediateptr;

    /* Fast cases */
    if (is_zero_uint(exponent, exponent_uint64_count)) {
        set_zero_poly(result_coeff_count, result_coeff_uint64_count, result);
        *result = 1;
        return;
    }

    if (is_equal_uint(exponent, exponent_uint64_count, 1)) {
        set_poly_poly(poly, poly_coeff_count, poly_coeff_uint64_count,
                result_coeff_count, result_coeff_uint64_count, result);
        return;
    }

    /* Need to make a copy of exponent */
    set_uint_uint(exponent, exponent_uint64_count, exponent_copy);

    /* Perform binary exponentiation. */
    productptr = (uint64_t *)get_poly_coeff(
                        (const uint64_t *)powerptr, result_coeff_count, result_coeff_uint64_count);
    intermediateptr = (uint64_t *)get_poly_coeff(
                        (const uint64_t *)productptr, result_coeff_count, result_coeff_uint64_count);

    set_poly_poly((const uint64_t *)poly, poly_coeff_count, poly_coeff_uint64_count, result_coeff_count,
                result_coeff_uint64_count, powerptr);
    set_zero_poly(result_coeff_count, result_coeff_uint64_count, intermediateptr);
    *intermediateptr = 1;

    /* Initially: power = operand and intermediate = 1, product is not initialized. */
    while (true) {
        if ((*exponent_copy % 2) == 1) {
            multiply_poly_poly(powerptr, result_coeff_count, result_coeff_uint64_count,
                    intermediateptr, result_coeff_count, result_coeff_uint64_count,
                    result_coeff_count, result_coeff_uint64_count, productptr);
            swap_ptr(&productptr, &intermediateptr);
        }
        right_shift_uint(exponent_copy, 1, exponent_uint64_count, exponent_copy);
        if (is_zero_uint(exponent_copy, exponent_uint64_count)) {
            break;
        }
        multiply_poly_poly(powerptr, result_coeff_count, result_coeff_uint64_count,
                powerptr, result_coeff_count, result_coeff_uint64_count,
                result_coeff_count, result_coeff_uint64_count, productptr);
        swap_ptr(&productptr, &powerptr);
    }
    set_poly_poly_simple(intermediateptr, result_coeff_count, result_coeff_uint64_count, result);
}

#ifdef __cplusplus
}
#endif