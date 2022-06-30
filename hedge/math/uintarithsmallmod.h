#ifndef __UINTARITHSMALLMOD_H__
#define __UINTARITHSMALLMOD_H__

#include "lib/types.h"
#include "math/smallmodulus.h"
#include "math/uintcore.h"
#include "math/uintarith.h"
#include "math/numth.h"

static inline uint64_t increment_uint_smallmod(uint64_t operand,
            const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (modulus->value == 0)
    {
        invalid_argument("modulus");
    }
    if (operand >= modulus->value)
    {
        out_of_range("operand");
    }
#endif
    operand++;
    return operand - (modulus->value & (uint64_t)(-(int64_t)(operand >= modulus->value)));
}

static inline uint64_t decrement_uint_smallmod(uint64_t operand,
    const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (modulus->value == 0)
    {
        invalid_argument("modulus");
    }
    if (operand >= modulus->value)
    {
        out_of_range("operand");
    }
#endif
    int64_t carry = (operand == 0);
    return operand - 1 + (modulus->value & (uint64_t)(-carry));
}

static inline uint64_t negate_uint_smallmod(uint64_t operand,
    const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (modulus->value == 0)
    {
        invalid_argument("modulus");
    }
    if (operand >= modulus->value)
    {
        out_of_range("operand");
    }
#endif
    int64_t non_zero = (operand != 0);
    return (modulus->value - operand) & (uint64_t)(-non_zero);
}

static inline uint64_t div2_uint_smallmod(uint64_t operand,
    const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (modulus->value == 0)
    {
        invalid_argument("modulus");
    }
    if (operand >= modulus->value)
    {
        out_of_range("operand");
    }
#endif
    if (operand & 1)
    {
        uint64_t temp;
        int64_t carry = add_uint64_w_carry(operand, modulus->value, 0, &temp);
        operand = temp >> 1;
        if (carry)
        {
            return operand | ((uint64_t)(1) << (BITS_PER_UINT64 - 1));
        }
        return operand;
    }
    return operand >> 1;
}

static inline uint64_t add_uint_uint_smallmod(uint64_t operand1,
    uint64_t operand2, const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (modulus->value == 0)
    {
        invalid_argument("modulus");
    }
    if (operand1 >= modulus->value)
    {
        out_of_range("operand1");
    }
    if (operand2 >= modulus->value)
    {
        out_of_range("operand2");
    }
#endif
    // Sum of operands modulo Zmodulus can never wrap around 2^64
    operand1 += operand2;
    return operand1 - (modulus->value & (uint64_t)(-(int64_t)(operand1 >= modulus->value)));
}

static inline uint64_t sub_uint_uint_smallmod(uint64_t operand1,
    uint64_t operand2, const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (modulus->value == 0)
    {
        invalid_argument("modulus");
    }

    if (operand1 >= modulus->value)
    {
        out_of_range("operand1");
    }
    if (operand2 >= modulus->value)
    {
        out_of_range("operand2");
    }
#endif
    uint64_t temp;
    int64_t borrow = sub_uint64_generic(operand1, operand2, 0, &temp);
    return (uint64_t)(temp) + (modulus->value & (uint64_t)(-borrow));
}

static inline uint64_t barrett_reduce_128(const uint64_t* input,
    const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (!input)
    {
        invalid_argument("input");
    }
    if (modulus->value == 0)
    {
        invalid_argument("modulus");
    }
#endif
    // Reduces input using base 2^64 Barrett reduction
    // input allocation size must be 128 bits

    uint64_t tmp1, tmp2[2], tmp3, carry;
    const uint64_t *const_ratio = modulus->const_ratio;

    //log_info("@@ const_ratio = {%lu %lu %lu}\n", const_ratio[0], const_ratio[1], const_ratio[2]);
    // Multiply input and const_ratio
    // Round 1
    multiply_uint64_hw64(input[0], const_ratio[0], &carry);
    log_verbose_extreme("barrett_reduce_128(%lu, %lu) const_ratio=%lu, carry=%lu\n",
        input[0], modulus->value, const_ratio[0], carry);

    multiply_uint64(input[0], const_ratio[1], tmp2);
    log_verbose_extreme("multiply_uint64(%lu, %lu)=%lu %lu\n", input[0], const_ratio[1], tmp2[0], tmp2[1]);
    
    tmp3 = tmp2[1] + add_uint64_w_carry(tmp2[0], carry, 0, &tmp1);
    log_verbose_extreme("tmp3=%lu\n", tmp3);
   
    // Round 2
    multiply_uint64(input[1], const_ratio[0], tmp2);
    carry = tmp2[1] + add_uint64_w_carry(tmp1, tmp2[0], 0, &tmp1);

    // This is all we care about
    tmp1 = input[1] * const_ratio[1] + tmp3 + carry;

    // Barrett subtraction
    tmp3 = input[0] - tmp1 * modulus->value;

    log_verbose_extreme("tmp1=%lu tmp3=%lu modulus->value_=%lu\n", tmp1, tmp3, modulus->value);
    // One more subtraction is enough
    return (uint64_t)(tmp3) -
        (modulus->value & (uint64_t)(
            -(int64_t)(tmp3 >= modulus->value)));
}

static inline uint64_t barrett_reduce_63(uint64_t input,
    const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (modulus->value == 0)
    {
        invalid_argument("modulus");
    }
    if (input >> 63)
    {
        invalid_argument("input");
    }
#endif
    // Reduces input using base 2^64 Barrett reduction
    // input must be at most 63 bits

    uint64_t tmp[2];
    const uint64_t *const_ratio = modulus->const_ratio;
    multiply_uint64(input, const_ratio[1], tmp);

    // Barrett subtraction
    tmp[0] = input - tmp[1] * modulus->value;

    // One more subtraction is enough
    return (uint64_t)(tmp[0]) -
        (modulus->value & (uint64_t)(-(int64_t)(tmp[0] >= modulus->value)));
}

static inline uint64_t multiply_uint_uint_smallmod(uint64_t operand1,
    uint64_t operand2, const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (modulus->value == 0)
    {
        invalid_argument("modulus");
    }
#endif
    uint64_t z[2];
    multiply_uint64(operand1, operand2, z);

    log_verbose_extreme("multiply_uint64(%lu, %lu) = 0x%lx %lx\n", operand1, operand2, z[1], z[0]);
    return barrett_reduce_128((const uint64_t *)z, modulus);
}

static inline void modulo_uint_inplace(uint64_t *value,
    size_t value_uint64_count, const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (!value && value_uint64_count > 0)
    {
        invalid_argument("value");
    }
#endif
    if (value_uint64_count == 1)
    {
        value[0] %= modulus->value;
        return;
    }

    // Starting from the top, reduce always 128-bit blocks
    for (size_t i = value_uint64_count - 1; i--; )
    {
        value[i] = barrett_reduce_128(value + i, modulus);
        value[i + 1] = 0;
    }
}

static inline uint64_t modulo_uint(const uint64_t *value,
    size_t value_uint64_count, const Zmodulus *modulus)
{
#ifdef HEDGE_DEBUG
    if (!value && value_uint64_count)
    {
        invalid_argument("value");
    }
    if (!value_uint64_count)
    {
        invalid_argument("value_uint64_count");
    }
#endif
    if (value_uint64_count == 1)
    {
        // If value < modulus no operation is needed
        return *value % modulus->value;
    }

    uint64_t* value_copy = hedge_malloc(sizeof(uint64_t) * value_uint64_count);
    uint64_t ret;
    set_uint_uint(value, value_uint64_count, value_copy);

    // Starting from the top, reduce always 128-bit blocks
    for (size_t i = value_uint64_count - 1; i--; )
    {
        value_copy[i] = barrett_reduce_128(value_copy + i, modulus);
    }

    ret = value_copy[0];
    hedge_free(value_copy);
    return ret;
}

static inline bool try_invert_uint_smallmod(uint64_t operand,
    const Zmodulus *modulus, uint64_t *result)
{
    return try_mod_inverse(operand, modulus->value, result);
}

bool is_primitive_root(uint64_t root, uint64_t degree,
    const Zmodulus *prime_modulus);

// Try to find a primitive degree-th root of unity modulo small prime
// modulus, where degree must be a power of two.
bool try_primitive_root(uint64_t degree,
    const Zmodulus *prime_modulus, uint64_t *destination);

// Try to find the smallest (as integer) primitive degree-th root of
// unity modulo small prime modulus, where degree must be a power of two.
bool try_minimal_primitive_root(uint64_t degree,
    const Zmodulus *prime_modulus, uint64_t *destination);

uint64_t exponentiate_uint_smallmod(uint64_t operand,
    uint64_t exponent, const Zmodulus *modulus);

void divide_uint_uint_mod_inplace(uint64_t *numerator,
    const Zmodulus *modulus, size_t uint64_count,
    uint64_t *quotient);

uint64_t steps_to_galois_elt(int steps, size_t coeff_count);

#endif /* __UINTARITHSMALLMOD_H__ */