#ifndef __UNITARITH_H__
#define __UNITARITH_H__

#include "lib/types.h"
#include "math/uintcore.h"
#if 0
#if !defined(__aarch64__) /* #if defined(__i386__) || defined(__x86_64__) */
#include <x86intrin.h>
#endif
#endif

static inline unsigned char sub_uint64_generic(uint64_t operand1, uint64_t operand2,
    unsigned char borrow, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!result) {
        invalid_argument("result cannot be null");
    }
#endif
    uint64_t diff = operand1 - operand2;
    *result = diff - (borrow != 0);
    return (diff > operand1) || (diff < borrow);
}

static inline unsigned char add_uint64_generic(uint64_t operand1, uint64_t operand2,
            unsigned char carry, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!result) {
        invalid_argument("result cannot be null");
    }
#endif
    operand1 += operand2;
    *result = operand1 + carry;
    return (operand1 < operand2) || (~operand1 < carry);
}

static inline unsigned char add_uint64(uint64_t operand1, size_t operand2, uint64_t *result)
{
    *result = operand1 + operand2;
    return (unsigned char)(*result < operand1);
}

static inline unsigned char add_uint64_w_carry(uint64_t operand1, uint64_t operand2,
            unsigned char carry, uint64_t *result)
{
    return add_uint64_generic(operand1, operand2, carry, result);
}

static inline unsigned char add_uint_uint_w_carry(const uint64_t *operand1,
    size_t operand1_uint64_count, const uint64_t *operand2,
    size_t operand2_uint64_count, unsigned char carry,
    size_t result_uint64_count, uint64_t *result)
{
    for (size_t i = 0; i < result_uint64_count; i++) {
        uint64_t temp_result;
        carry = add_uint64_w_carry(
            (i < operand1_uint64_count) ? *operand1++ : 0,
            (i < operand2_uint64_count) ? *operand2++ : 0,
            carry, &temp_result);
        *result++ = temp_result;
    }
    return carry;
}

static inline unsigned char add_uint_uint(const uint64_t *operand1,
            const uint64_t *operand2, size_t uint64_count,
            uint64_t *result)
{
    // Unroll first iteration of loop. We assume uint64_count > 0.
    unsigned char carry = add_uint64(*operand1++, *operand2++, result++);
    // Do the rest
    for(; --uint64_count; operand1++, operand2++, result++) {
        uint64_t temp_result;
        carry = add_uint64_w_carry(*operand1, *operand2, carry, &temp_result);
        //print_array_info(result, 3);
        *result = temp_result;
    }

    return carry;
}

static inline unsigned char add_uint_uint64(const uint64_t *operand1,
    uint64_t operand2, size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!uint64_count) {
        invalid_argument("uint64_count");
    }
    if (!operand1) {
        invalid_argument("operand1");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    // Unroll first iteration of loop. We assume uint64_count > 0.
    unsigned char carry = add_uint64(*operand1++, operand2, result++);

    // Do the rest
    for(; --uint64_count; operand1++, result++) {
        uint64_t temp_result;
        carry = add_uint64_w_carry(*operand1, (uint64_t)(0), carry, &temp_result);
        *result = temp_result;
    }
    return carry;
}

static inline unsigned char sub_uint64_w_borrow(uint64_t operand1, uint64_t operand2,
    unsigned char borrow, uint64_t *result)
{
    return sub_uint64_generic(operand1, operand2, borrow, result);
}

static inline unsigned char sub_uint64(uint64_t operand1, uint64_t operand2, uint64_t *result)
{
    *result = operand1 - operand2;
    return (unsigned char)(operand2 > operand1);
}

static inline unsigned char sub_uint_uint_w_borrow(const uint64_t *operand1,
    size_t operand1_uint64_count, const uint64_t *operand2,
    size_t operand2_uint64_count, unsigned char borrow,
    size_t result_uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!result_uint64_count) {
        invalid_argument("result_uint64_count");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    for (size_t i = 0; i < result_uint64_count;
        i++, operand1++, operand2++, result++) {
        uint64_t temp_result;
        borrow = sub_uint64_w_borrow((i < operand1_uint64_count) ? *operand1 : 0,
            (i < operand2_uint64_count) ? *operand2 : 0, borrow, &temp_result);
        *result = temp_result;
    }
    return borrow;
}

static inline unsigned char sub_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!uint64_count) {
        invalid_argument("uint64_count");
    }
    if (!operand1) {
        invalid_argument("operand1");
    }
    if (!operand2) {
        invalid_argument("operand2");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif    
    log_verbose("***** START sub_uint_uint *****\n");
    print_array_verbose(operand1, uint64_count);
    print_array_verbose(operand2, uint64_count);

    unsigned char borrow;

    if (uint64_count % 2 == 1) {
        borrow = sub_uint64(*operand1++, *operand2++, result++);
        uint64_count--;
        do {
            borrow = sub_uint64_w_borrow(*operand1++, *operand2++, borrow, result++);
            borrow = sub_uint64_w_borrow(*operand1++, *operand2++, borrow, result++);
        } while(uint64_count -= 2);
    } else {
        do {
            borrow = sub_uint64_w_borrow(*operand1++, *operand2++, borrow, result++);
            borrow = sub_uint64_w_borrow(*operand1++, *operand2++, borrow, result++);
        } while (uint64_count -= 2);
    }
    log_verbose("****** END sub_uint_uint *****\n");
    return borrow;
}

static inline unsigned char sub_uint_uint64(const uint64_t *operand1,
    uint64_t operand2, size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!uint64_count) {
        invalid_argument("uint64_count");
    }
    if (!operand1) {
        invalid_argument("operand1");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    // Unroll first iteration of loop. We assume uint64_count > 0.
    unsigned char borrow = sub_uint64(*operand1++, operand2, result++);

    // Do the rest
    for(; --uint64_count; operand1++, operand2++, result++) {
        uint64_t temp_result;
        borrow = sub_uint64_w_borrow(*operand1, (uint64_t)(0), borrow, &temp_result);
        *result = temp_result;
    }
    return borrow;
}

static inline unsigned char increment_uint(const uint64_t *operand,
    size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand) {
        invalid_argument("operand");
    }
    if (!uint64_count) {
        invalid_argument("uint64_count");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    return add_uint_uint64(operand, 1, uint64_count, result);
}

static inline unsigned char decrement_uint(const uint64_t *operand,
    size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand && uint64_count > 0) {
        invalid_argument("operand");
    }
    if (!result && uint64_count > 0) {
        invalid_argument("result");
    }
#endif
    return sub_uint_uint64(operand, 1, uint64_count, result);
}

static inline void negate_uint(const uint64_t *operand, size_t uint64_count,
    uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand) {
        invalid_argument("operand");
    }
    if (!uint64_count) {
        invalid_argument("uint64_count");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif

#if defined(__aarch64__)
    if (uint64_count % 2 == 1) {
        __asm volatile ("negs %[o], %[i]" : [o] "=r" (*result++) : [i] "r" (*operand++));
        uint64_count--;
        do {
            __asm volatile ("ngcs %[o], %[i]" : [o] "=r" (*result++) : [i] "r" (*operand++));
            __asm volatile ("ngcs %[o], %[i]" : [o] "=r" (*result++) : [i] "r" (*operand++));
        } while (uint64_count -= 2);
    } else {
        do {
            __asm volatile ("ngcs %[o], %[i]" : [o] "=r" (*result++) : [i] "r" (*operand++));
            __asm volatile ("ngcs %[o], %[i]" : [o] "=r" (*result++) : [i] "r" (*operand++));
        } while (uint64_count -= 2);
    }
#else
    // Negation is equivalent to inverting bits and adding 1.
    unsigned char carry = add_uint64(~*operand++, (uint64_t)(1), result++);
    for(; --uint64_count; operand++, result++) {
        uint64_t temp_result;
        carry = add_uint64_w_carry(
            ~*operand, (uint64_t)(0), carry, &temp_result);
        *result = temp_result;
    }
#endif
}

static inline void left_shift_uint(const uint64_t *operand,
    int shift_amount, size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand) {
        invalid_argument("operand");
    }
    if (shift_amount < 0 ||
        unsigned_geq(shift_amount,
            mul_safe(uint64_count, BITS_PER_UINT64))) {
        invalid_argument("shift_amount");
    }
    if (!uint64_count) {
        invalid_argument("uint64_count");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    // How many words to shift
    size_t uint64_shift_amount =
        (size_t)(shift_amount) / BITS_PER_UINT64;

    // Shift words
    for (size_t i = 0; i < uint64_count - uint64_shift_amount; i++) {
        result[uint64_count - i - 1] = operand[uint64_count - i - 1 - uint64_shift_amount];
    }
    for (size_t i = uint64_count - uint64_shift_amount; i < uint64_count; i++) {
        result[uint64_count - i - 1] = 0;
    }

    // How many bits to shift in addition
    size_t bit_shift_amount = (size_t)(shift_amount)
        - (uint64_shift_amount * BITS_PER_UINT64);

    if (bit_shift_amount) {
        size_t neg_bit_shift_amount = BITS_PER_UINT64 - bit_shift_amount;

        for (size_t i = uint64_count - 1; i > 0; i--) {
            result[i] = (result[i] << bit_shift_amount) |
                (result[i - 1] >> neg_bit_shift_amount);
        }
        result[0] = result[0] << bit_shift_amount;
    }
}

static inline void right_shift_uint(const uint64_t *operand,
    int shift_amount, size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand) {
        invalid_argument("operand");
    }
    if (shift_amount < 0 ||
        unsigned_geq(shift_amount,
            mul_safe(uint64_count, BITS_PER_UINT64))) {
        invalid_argument("shift_amount");
    }
    if (!uint64_count) {
        invalid_argument("uint64_count");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    // How many words to shift
    size_t uint64_shift_amount =
        (size_t)(shift_amount) / BITS_PER_UINT64;

    // Shift words
    for (size_t i = 0; i < uint64_count - uint64_shift_amount; i++) {
        result[i] = operand[i + uint64_shift_amount];
    }
    for (size_t i = uint64_count - uint64_shift_amount; i < uint64_count; i++) {
        result[i] = 0;
    }

    // How many bits to shift in addition
    size_t bit_shift_amount = (size_t)(shift_amount)
        - (uint64_shift_amount * BITS_PER_UINT64);

    if (bit_shift_amount) {
        size_t neg_bit_shift_amount = BITS_PER_UINT64 - bit_shift_amount;

        for (size_t i = 0; i < uint64_count - 1; i++) {
            result[i] = (result[i] >> bit_shift_amount) |
                (result[i + 1] << neg_bit_shift_amount);
        }
        result[uint64_count - 1] = result[uint64_count - 1] >> bit_shift_amount;
    }
}

static inline void left_shift_uint128(
    const uint64_t *operand, int shift_amount, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand) {
        invalid_argument("operand");
    }
    if (shift_amount < 0 ||
        unsigned_geq(shift_amount, 2 * BITS_PER_UINT64)) {
        invalid_argument("shift_amount");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    const size_t shift_amount_sz = (size_t)(shift_amount);

    // Early return
    if (shift_amount_sz & BITS_PER_UINT64) {
        result[1] = operand[0];
        result[0] = 0;
    } else {
        result[1] = operand[1];
        result[0] = operand[0];
    }

    // How many bits to shift in addition to word shift
    size_t bit_shift_amount = shift_amount_sz & (BITS_PER_UINT64 - 1);

    // Do we have a word shift
    if (bit_shift_amount) {
        size_t neg_bit_shift_amount = BITS_PER_UINT64 - bit_shift_amount;

        // Warning: if bit_shift_amount == 0 this is incorrect
        result[1] = (result[1] << bit_shift_amount) |
            (result[0] >> neg_bit_shift_amount);
        result[0] = result[0] << bit_shift_amount;
    }
}

static inline void right_shift_uint128(
    const uint64_t *operand, int shift_amount, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand) {
        invalid_argument("operand");
    }
    if (shift_amount < 0 ||
        unsigned_geq(shift_amount, 2 * BITS_PER_UINT64)) {
        invalid_argument("shift_amount");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    const size_t shift_amount_sz = (size_t)(shift_amount);

    if (shift_amount_sz & BITS_PER_UINT64) {
        result[0] = operand[1];
        result[1] = 0;
    } else {
        result[1] = operand[1];
        result[0] = operand[0];
    }

    // How many bits to shift in addition to word shift
    size_t bit_shift_amount = shift_amount_sz & (BITS_PER_UINT64 - 1);

    if (bit_shift_amount) {
        size_t neg_bit_shift_amount = BITS_PER_UINT64 - bit_shift_amount;

        // Warning: if bit_shift_amount == 0 this is incorrect
        result[0] = (result[0] >> bit_shift_amount) |
            (result[1] << neg_bit_shift_amount);
        result[1] = result[1] >> bit_shift_amount;
    }
}

static inline void left_shift_uint192(
    const uint64_t *operand, int shift_amount, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand) {
        invalid_argument("operand");
    }
    if (shift_amount < 0 ||
        unsigned_geq(shift_amount, 3 * BITS_PER_UINT64)) {
        invalid_argument("shift_amount");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    const size_t shift_amount_sz = (size_t)(shift_amount);

    if (shift_amount_sz & (BITS_PER_UINT64 << 1)) {
        result[2] = operand[0];
        result[1] = 0;
        result[0] = 0;
    } else if (shift_amount_sz & BITS_PER_UINT64) {
        result[2] = operand[1];
        result[1] = operand[0];
        result[0] = 0;
    } else {
        result[2] = operand[2];
        result[1] = operand[1];
        result[0] = operand[0];
    }

    // How many bits to shift in addition to word shift
    size_t bit_shift_amount = shift_amount_sz & (BITS_PER_UINT64 - 1);

    if (bit_shift_amount) {
        size_t neg_bit_shift_amount = BITS_PER_UINT64 - bit_shift_amount;

        // Warning: if bit_shift_amount == 0 this is incorrect
        result[2] = (result[2] << bit_shift_amount) |
            (result[1] >> neg_bit_shift_amount);
        result[1] = (result[1] << bit_shift_amount) |
            (result[0] >> neg_bit_shift_amount);
        result[0] = result[0] << bit_shift_amount;
    }
}

static inline void right_shift_uint192(
    const uint64_t *operand, int shift_amount, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand) {
        invalid_argument("operand");
    }
    if (shift_amount < 0 ||
        unsigned_geq(shift_amount, 3 * BITS_PER_UINT64)) {
        invalid_argument("shift_amount");
    }
    if (!result) {
        invalid_argument("result");
    }
#endif
    const size_t shift_amount_sz = (size_t)(shift_amount);

    if (shift_amount_sz & (BITS_PER_UINT64 << 1)) {
        result[0] = operand[2];
        result[1] = 0;
        result[2] = 0;
    } else if (shift_amount_sz & BITS_PER_UINT64) {
        result[0] = operand[1];
        result[1] = operand[2];
        result[2] = 0;
    } else {
        result[2] = operand[2];
        result[1] = operand[1];
        result[0] = operand[0];
    }

    // How many bits to shift in addition to word shift
    size_t bit_shift_amount = shift_amount_sz & (BITS_PER_UINT64 - 1);

    if (bit_shift_amount) {
        size_t neg_bit_shift_amount = BITS_PER_UINT64 - bit_shift_amount;

        // Warning: if bit_shift_amount == 0 this is incorrect
        result[0] = (result[0] >> bit_shift_amount) |
            (result[1] << neg_bit_shift_amount);
        result[1] = (result[1] >> bit_shift_amount) |
            (result[2] << neg_bit_shift_amount);
        result[2] = result[2] >> bit_shift_amount;
    }
}

static inline void half_round_up_uint(const uint64_t *operand,
    size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand && uint64_count > 0) {
        invalid_argument("operand");
    }
    if (!result && uint64_count > 0) {
        invalid_argument("result");
    }
#endif
    if (!uint64_count) {
        return;
    }
    // Set result to (operand + 1) / 2. To prevent overflowing operand, right shift
    // and then increment result if low-bit of operand was set.
    bool low_bit_set = operand[0] & 1;

    for (size_t i = 0; i < uint64_count - 1; i++) {
        result[i] = (operand[i] >> 1) | (operand[i + 1] << (BITS_PER_UINT64 - 1));
    }
    result[uint64_count - 1] = operand[uint64_count - 1] >> 1;

    if (low_bit_set) {
        increment_uint(result, uint64_count, result);
    }
}

static inline void not_uint(const uint64_t *operand, size_t uint64_count,
    uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand && uint64_count > 0) {
        invalid_argument("operand");
    }
    if (!result && uint64_count > 0) {
        invalid_argument("result");
    }
#endif
    for (; uint64_count--; result++, operand++) {
        *result = ~*operand;
    }
}

static inline void and_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand1 && uint64_count > 0) {
        invalid_argument("operand1");
    }
    if (!operand2 && uint64_count > 0) {
        invalid_argument("operand2");
    }
    if (!result && uint64_count > 0) {
        invalid_argument("result");
    }
#endif
    for (; uint64_count--; result++, operand1++, operand2++) {
        *result = *operand1 & *operand2;
    }
}

static inline void or_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand1 && uint64_count > 0) {
        invalid_argument("operand1");
    }
    if (!operand2 && uint64_count > 0) {
        invalid_argument("operand2");
    }
    if (!result && uint64_count > 0) {
        invalid_argument("result");
    }
#endif
    for (; uint64_count--; result++, operand1++, operand2++) {
        *result = *operand1 | *operand2;
    }
}

static inline void xor_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!operand1 && uint64_count > 0) {
        invalid_argument("operand1");
    }
    if (!operand2 && uint64_count > 0) {
        invalid_argument("operand2");
    }
    if (!result && uint64_count > 0) {
        invalid_argument("result");
    }
#endif
    for (; uint64_count--; result++, operand1++, operand2++) {
        *result = *operand1 ^ *operand2;
    }
}

static inline void multiply_uint64_generic(uint64_t operand1, uint64_t operand2,
    uint64_t *result128)
{
#ifdef HEDGE_DEBUG
    if (!result128) {
        invalid_argument("result128 cannot be null");
    }
#endif

#if defined(__aarch64__)
    result128[0] = operand1 * operand2;
    __asm volatile("umulh %[x_d], %[x_n], %[x_m]"
        : [x_d] "=r" (result128[1])
        : [x_n] "r" (operand1), [x_m] "r" (operand2));
#else
    uint64_t operand1_coeff_right = operand1 & 0x00000000FFFFFFFF;
    uint64_t operand2_coeff_right = operand2 & 0x00000000FFFFFFFF;
    operand1 >>= 32;
    operand2 >>= 32;

    uint64_t middle1 = operand1 * operand2_coeff_right;
    uint64_t middle;
    uint64_t left = operand1 * operand2 + ((uint64_t)(add_uint64(
        middle1, operand2 * operand1_coeff_right, &middle)) << 32);
    uint64_t right = operand1_coeff_right * operand2_coeff_right;
    uint64_t temp_sum = (right >> 32) + (middle & 0x00000000FFFFFFFF);

    result128[1] = (uint64_t)(left + (middle >> 32) + (temp_sum >> 32));
    result128[0] = (uint64_t)((temp_sum << 32) | (right & 0x00000000FFFFFFFF));
#endif
}

#ifdef HEDGE_TARGET_HAS_128_SUPPORT
static inline void multiply_uint64_native(uint64_t operand1, uint64_t operand2,
    uint64_t *result128)
{
    uint128_t result = (uint128_t)operand1 * (uint128_t)operand2;
    result128[1] = (uint64_t)(result >> 64);
    result128[0] = (uint64_t)(result) ;
}
#endif

static inline void multiply_uint64(uint64_t operand1, uint64_t operand2,
    uint64_t *result128)
{
#ifdef HEDGE_TARGET_HAS_128_SUPPORT
    multiply_uint64_native(operand1, operand2, result128);
#else
    multiply_uint64_generic(operand1, operand2, result128);
#endif
}

static inline void multiply_uint64_hw64_generic(uint64_t operand1, uint64_t operand2,
    uint64_t *hw64)
{
#ifdef HEDGE_DEBUG
    if (!hw64) {
        invalid_argument("hw64 cannot be null");
    }
#endif

#if defined(__aarch64__)
    __asm volatile("umulh %[x_d], %[x_n], %[x_m]"
        : [x_d] "=r" (*hw64)
        : [x_n] "r" (operand1), [x_m] "r" (operand2));
#else
    uint64_t operand1_coeff_right = operand1 & 0x00000000FFFFFFFF;
    uint64_t operand2_coeff_right = operand2 & 0x00000000FFFFFFFF;
    operand1 >>= 32;
    operand2 >>= 32;

    uint64_t middle1 = operand1 * operand2_coeff_right;
    uint64_t middle;
    uint64_t left = operand1 * operand2 + ((uint64_t)(add_uint64(
        middle1, operand2 * operand1_coeff_right, &middle)) << 32);
    uint64_t right = operand1_coeff_right * operand2_coeff_right;
    uint64_t temp_sum = (right >> 32) + (middle & 0x00000000FFFFFFFF);

    *hw64 = (uint64_t)(left + (middle >> 32) + (temp_sum >> 32));
    log_verbose_extreme("middle1=%lu middle=%lu right=%lu temp_sum=%lu left=%lu\n",
        middle1, middle, right, temp_sum, left);
#endif
}

static inline void multiply_uint64_hw64(uint64_t operand1, uint64_t operand2,
    uint64_t *hw64)
{
    multiply_uint64_hw64_generic(operand1, operand2, hw64);
}

void multiply_uint_uint_6(const uint64_t *operand1,
    size_t operand1_uint64_count, const uint64_t *operand2,
    size_t operand2_uint64_count, size_t result_uint64_count,
    uint64_t *result);

static inline void multiply_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count, uint64_t *result)
{
    multiply_uint_uint_6(operand1, uint64_count, operand2, uint64_count,
        uint64_count * 2, result);
}

void multiply_uint_uint64(const uint64_t *operand1,
    size_t operand1_uint64_count, uint64_t operand2,
    size_t result_uint64_count, uint64_t *result);

static inline void multiply_truncate_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count, uint64_t *result)
{
    multiply_uint_uint_6(operand1, uint64_count, operand2, uint64_count,
        uint64_count, result);
}

void divide_uint_uint_inplace(uint64_t *numerator,
    const uint64_t *denominator, size_t uint64_count,
    uint64_t *quotient);

static inline void divide_uint_uint(const uint64_t *numerator,
    const uint64_t *denominator, size_t uint64_count,
    uint64_t *quotient, uint64_t *remainder)
{
    set_uint_uint(numerator, uint64_count, remainder);
    divide_uint_uint_inplace(remainder, denominator, uint64_count, quotient);
}

void divide_uint128_uint64_inplace_generic(uint64_t *numerator,
    uint64_t denominator, uint64_t *quotient);

static inline void divide_uint128_uint64_inplace(uint64_t *numerator,
    uint64_t denominator, uint64_t *quotient)
{
#ifdef HEDGE_DEBUG
    if (!numerator) {
        invalid_argument("numerator");
    }
    if (denominator == 0) {
        invalid_argument("denominator");
    }
    if (!quotient) {
        invalid_argument("quotient");
    }
    if (numerator == quotient) {
        invalid_argument("quotient cannot point to same value as numerator");
    }
#endif
    divide_uint128_uint64_inplace_generic(numerator, denominator, quotient);
}

void divide_uint128_uint64_inplace(uint64_t *numerator,
    uint64_t denominator, uint64_t *quotient);

void divide_uint192_uint64_inplace(uint64_t *numerator,
    uint64_t denominator, uint64_t *quotient);

void exponentiate_uint(const uint64_t *operand,
    size_t operand_uint64_count, const uint64_t *exponent,
    size_t exponent_uint64_count, size_t result_uint64_count,
    uint64_t *result);

uint64_t exponentiate_uint64_safe(uint64_t operand,
    uint64_t exponent);

uint64_t exponentiate_uint64(uint64_t operand,
    uint64_t exponent);

#endif /* __UNITARITH_H__ */
