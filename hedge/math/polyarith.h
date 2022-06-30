#ifndef __POLYARITH_H__
#define __POLYARITH_H__

#include <assert.h>

#include "math/uintcore.h"
#include "math/uintarith.h"
#include "math/uintarithmod.h"
#include "math/polycore.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void right_shift_poly_coeffs(const uint64_t *poly,
                                    size_t coeff_count,
                                    size_t coeff_uint64_count,
                                    int shift_amount,
                                    uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(poly || (!coeff_count && !coeff_uint64_count));
#endif
    while (coeff_count--) {
        right_shift_uint(poly, shift_amount, coeff_uint64_count, result);
        poly += coeff_uint64_count;
        result += coeff_uint64_count;
    }
}

static inline void negate_poly(const uint64_t *poly,
                        size_t coeff_count,
                        size_t coeff_uint64_count,
                        uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(poly || (!coeff_count && !coeff_uint64_count));
    assert(result || (!coeff_count && !coeff_uint64_count));
#endif
    while (coeff_count--) {
        negate_uint(poly, coeff_uint64_count, result);
        poly += coeff_uint64_count;
        result += coeff_uint64_count;
    }
}

static inline void add_poly_poly(const uint64_t *operand1,
                          const uint64_t *operand2,
                          size_t coeff_count,
                          size_t coeff_uint64_count,
                          uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(operand1 || (!coeff_count && !coeff_uint64_count));
    assert(operand2 || (!coeff_count && !coeff_uint64_count));
    assert(result || (!coeff_count && !coeff_uint64_count));
#endif
    while (coeff_count--) {
        add_uint_uint(operand1, operand2, coeff_uint64_count, result);
        operand1 += coeff_uint64_count;
        operand2 += coeff_uint64_count;
        result += coeff_uint64_count;
    }
}

static inline void sub_poly_poly(const uint64_t *operand1,
                          const uint64_t *operand2,
                          size_t coeff_count,
                          size_t coeff_uint64_count,
                          uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(operand1 || (!coeff_count && !coeff_uint64_count));
    assert(operand2 || (!coeff_count && !coeff_uint64_count));
    assert(result || (!coeff_count && !coeff_uint64_count));
#endif
    while (coeff_count--) {
        sub_uint_uint(operand1, operand2, coeff_uint64_count, result);
        operand1 += coeff_uint64_count;
        operand2 += coeff_uint64_count;
        result += coeff_uint64_count;
    }
}

static inline void poly_infty_norm(const uint64_t *poly,
                            size_t coeff_count,
                            size_t coeff_uint64_count,
                            uint64_t *result)
{
    set_zero_uint(coeff_uint64_count, result);
    while (coeff_count--) {
        if (is_greater_than_uint_uint(poly, result, coeff_uint64_count)) {
            set_uint_uint(poly, coeff_uint64_count, result);
        }
        poly += coeff_uint64_count;
    }
}

extern void multiply_poly_poly(
        const uint64_t *operand1,
        size_t operand1_coeff_count,
        size_t operand1_coeff_uint64_count,
        const uint64_t *operand2,
        size_t operand2_coeff_count,
        size_t operand2_coeff_uint64_count,
        size_t result_coeff_count,
        size_t result_coeff_uint64_count,
        uint64_t *result);

extern void poly_eval_poly(
        const uint64_t *poly_to_eval,
        size_t poly_to_eval_coeff_count,
        size_t poly_to_eval_coeff_uint64_count,
        const uint64_t *value,
        size_t value_coeff_count,
        size_t value_coeff_uint64_count,
        size_t result_coeff_count,
        size_t result_coeff_uint64_count,
        uint64_t *result);

extern void exponentiate_poly(
        const uint64_t *poly,
        size_t poly_coeff_count,
        size_t poly_coeff_uint64_count,
        const uint64_t *exponent,
        size_t exponent_uint64_count,
        size_t result_coeff_count,
        size_t result_coeff_uint64_count,
        uint64_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __POLYARITH_H__ */
