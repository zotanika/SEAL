#ifndef __POLYARITHSMALLMOD_H__
#define __POLYARITHSMALLMOD_H__

#include <assert.h>

#include "lib/algorithm.h"
#include "lib/common.h"
#include "math/uintarithmod.h"
#include "math/polycore.h"
#include "math/smallmodulus.h"
#include "math/uintarithsmallmod.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void modulo_poly_coeffs(
        const uint64_t *poly,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

extern void modulo_poly_coeffs_63(
        const uint64_t *poly,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

extern void negate_poly_coeffsmallmod(
        const uint64_t *poly,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

extern void add_poly_poly_coeffsmallmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

extern void sub_poly_poly_coeffsmallmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

extern void multiply_poly_scalar_coeffsmallmod(
        const uint64_t *poly,
        size_t coeff_count,
        uint64_t scalar,
        const Zmodulus *modulus,
        uint64_t *result);

extern void multiply_poly_poly_coeffsmallmod(
        const uint64_t *operand1,
        size_t operand1_coeff_count,
        const uint64_t *operand2,
        size_t operand2_coeff_count,
        const Zmodulus *modulus,
        size_t result_coeff_count,
        uint64_t *result);

extern void multiply_poly_poly_coeffsmallmod_5(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

extern void multiply_truncate_poly_poly_coeffsmallmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

extern void divide_poly_poly_coeffmod_inplace(
        uint64_t *numerator,
        const uint64_t *denominator,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *quotient);

extern void divide_poly_poly_coeffsmallmod(
        const uint64_t *numerator,
        const uint64_t *denominator,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *quotient,
        uint64_t *remainder);

extern void apply_galois(
        const uint64_t *input,
        int coeff_count_power,
        uint64_t galois_elt,
        const Zmodulus *modulus,
        uint64_t *result);

extern void apply_galois_ntt(
        const uint64_t *input,
        int coeff_count_power,
        uint64_t galois_elt,
        uint64_t *result);

extern void dyadic_product_coeffsmallmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

extern uint64_t poly_infty_norm_coeffsmallmod(
        const uint64_t *operand,
        size_t coeff_count,
        const Zmodulus *modulus);

extern bool try_invert_poly_coeffsmallmod(
        const uint64_t *operand,
        const uint64_t *poly_modulus,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

extern void negacyclic_shift_poly_coeffsmallmod(
        const uint64_t *operand,
        size_t coeff_count,
        size_t shift,
        const Zmodulus *modulus,
        uint64_t *result);

extern void negacyclic_multiply_poly_mono_coeffsmallmod(
        const uint64_t *operand,
        size_t coeff_count,
        uint64_t mono_coeff,
        size_t mono_exponent,
        const Zmodulus *modulus,
        uint64_t *result);

extern void add_poly_poly_coeffsmallmod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        const Zmodulus *modulus,
        uint64_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __POLYARITHSMALLMOD_H__ */