#ifndef __POLYARITHMOD_H__
#define __POLYARITHMOD_H__

#include <assert.h>

#include "math/uintarithmod.h"
#include "math/polycore.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void negate_poly_coeffmod(
        const uint64_t *poly,
        size_t coeff_count,
        const uint64_t *coeff_modulus,
        size_t coeff_uint64_count,
        uint64_t *result)
{
    size_t i;

#ifdef HEDGE_DEBUG
    assert(poly || !coeff_count);
    assert(coeff_modulus);
    assert(coeff_uint64_count);
    assert(result || !coeff_count);
#endif
    for (i = 0; i < coeff_count; i++) {
        negate_uint_mod(poly, coeff_modulus, coeff_uint64_count, result);
        poly += coeff_uint64_count;
        result += coeff_uint64_count;
    }
}

static inline void add_poly_poly_coeffmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const uint64_t *coeff_modulus,
        size_t coeff_uint64_count,
        uint64_t *result)
{
    size_t i;

#ifdef HEDGE_DEBUG
    assert(operand1 || !coeff_count);
    assert(operand2 || !coeff_count);
    assert(coeff_modulus);
    assert(coeff_uint64_count);
    assert(result || !coeff_count);
#endif
    for (i = 0; i < coeff_count; i++) {
        add_uint_uint_mod(operand1, operand2, coeff_modulus,
                coeff_uint64_count, result);
        operand1 += coeff_uint64_count;
        operand2 += coeff_uint64_count;
        result += coeff_uint64_count;
    }
}

static inline void sub_poly_poly_coeffmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const uint64_t *coeff_modulus,
        size_t coeff_uint64_count,
        uint64_t *result)
{
    size_t i;

#ifdef HEDGE_DEBUG
    assert(operand1 || !coeff_count);
    assert(operand2 || !coeff_count);
    assert(coeff_modulus);
    assert(coeff_uint64_count);
    assert(result || !coeff_count);
#endif
    for (i = 0; i < coeff_count; i++) {
        sub_uint_uint_mod(operand1, operand2, coeff_modulus,
                coeff_uint64_count, result);
        operand1 += coeff_uint64_count;
        operand2 += coeff_uint64_count;
        result += coeff_uint64_count;
    }
}

extern void poly_infty_norm_coeffmod(
        const uint64_t *poly,
        size_t coeff_count,
        size_t coeff_uint64_count,
        const uint64_t *modulus,
        uint64_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __POLYARITHMOD_H__ */