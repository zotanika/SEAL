#ifndef __UINTARITHMOD_H__
#define __UINTARITHMOD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include "uintcore.h"
#include "uintarith.h"

static inline void increment_uint_mod(
        const uint64_t *operand,
        const uint64_t *modulus,
        size_t uint64_count,
        uint64_t *result)
{
    unsigned char carry;
#ifdef HEDGE_DEBUG
    assert(operand);
    assert(modulus);
    assert(uint64_count);
    assert(result);
    assert(is_less_than_uint_uint(operand, modulus, uint64_count));
#endif
    carry = increment_uint(operand, uint64_count, result);
    if (carry ||
        is_greater_than_or_equal_uint_uint(result, modulus, uint64_count)) {
        sub_uint_uint(result, modulus, uint64_count, result);
    }
}

static inline void decrement_uint_mod(
        const uint64_t *operand,
        const uint64_t *modulus,
        size_t uint64_count,
        uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(operand);
    assert(modulus);
    assert(uint64_count);
    assert(result);
    assert(is_less_than_uint_uint(operand, modulus, uint64_count));
#endif
    if (decrement_uint(operand, uint64_count, result)) {
        add_uint_uint(result, modulus, uint64_count, result);
    }
}

static inline void negate_uint_mod(
        const uint64_t *operand,
        const uint64_t *modulus,
        size_t uint64_count,
        uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(operand);
    assert(modulus);
    assert(uint64_count);
    assert(result);
    assert(is_less_than_uint_uint(operand, modulus, uint64_count));
#endif
    if (is_zero_uint(operand, uint64_count)) {
        set_zero_uint(uint64_count, result);
    } else {
        sub_uint_uint(modulus, operand, uint64_count, result);
    }
}

static inline void div2_uint_mod(
        const uint64_t *operand,
        const uint64_t *modulus,
        size_t uint64_count,
        uint64_t *result)
{
    unsigned char carry;
#ifdef HEDGE_DEBUG
    assert(operand);
    assert(modulus);
    assert(uint64_count);
    assert(result);
    assert(is_bit_set_uint(modulus, uint64_count, 0));
    assert(is_less_than_uint_uint(operand, modulus, uint64_count));
#endif
    if (*operand & 1) {
        carry = add_uint_uint(operand, modulus, uint64_count, result);
        right_shift_uint(result, 1, uint64_count, result);
        if (carry) {
            set_bit_uint(result, uint64_count, (int)uint64_count * BITS_PER_UINT64 - 1);
        }
    } else {
        right_shift_uint(operand, 1, uint64_count, result);
    }
}

static inline void add_uint_uint_mod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        const uint64_t *modulus,
        size_t uint64_count,
        uint64_t *result)
{
    unsigned char carry;
#ifdef HEDGE_DEBUG
    assert(operand1);
    assert(operand2);
    assert(modulus);
    assert(uint64_count);
    assert(result);
    assert(is_bit_set_uint(modulus, uint64_count, 0));
    assert(is_less_than_uint_uint(operand1, modulus, uint64_count));
    assert(is_less_than_uint_uint(operand2, modulus, uint64_count));
#endif
    carry = add_uint_uint(operand1, operand2, uint64_count, result);
    if (carry ||
        is_greater_than_or_equal_uint_uint(result, modulus, uint64_count)) {
        sub_uint_uint(result, modulus, uint64_count, result);
    }
}

static inline void sub_uint_uint_mod(
        const uint64_t *operand1,
        const uint64_t *operand2,
        const uint64_t *modulus,
        size_t uint64_count,
        uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(operand1);
    assert(operand2);
    assert(modulus);
    assert(uint64_count);
    assert(result);
    assert(is_less_than_uint_uint(operand1, modulus, uint64_count));
    assert(is_less_than_uint_uint(operand2, modulus, uint64_count));
#endif
    if (sub_uint_uint(operand1, operand2, uint64_count, result)) {
        add_uint_uint(result, modulus, uint64_count, result);
    }
}

extern bool try_invert_uint_mod(
        const uint64_t *operand,
        const uint64_t *modulus,
        size_t uint64_count,
        uint64_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __UINTARITHMOD_H__ */