#ifndef __UINTCORE_H__
#define __UINTCORE_H__

#include <limits.h>
#include "lib/types.h"
#include "lib/algorithm.h"
#include "lib/common.h"
#include "defines.h"

char* uint_to_hex_string(const uint64_t* value, size_t uint64_count);

char* uint_to_dec_string(const uint64_t* value, size_t uint64_count);

void hex_string_to_uint(char* hex_string, int char_count, size_t uint64_count, uint64_t *result);


static inline uint64_t* allocate_uint(size_t uint64_count)
{
    return hedge_malloc(sizeof(uint64_t) * uint64_count);
}
static inline uint64_t* zallocate_uint(size_t uint64_count)
{
    return hedge_zalloc(sizeof(uint64_t) * uint64_count);
}

static inline void set_zero_uint(size_t uint64_count, uint64_t* result)
{
    memset(result, 0, uint64_count * sizeof(uint64_t));
}

static inline uint64_t* allocate_zero_uint(size_t uint64_count)
{
    return hedge_zalloc(sizeof(uint64_t) * uint64_count);
}

static inline void set_uint(uint64_t value, size_t uint64_count, uint64_t *result)
{
    *result++ = value;
    for (; --uint64_count; result++)
    {
        *result = 0;
    }
}

static inline bool is_zero_uint(const uint64_t *value, size_t uint64_count)
{
    while (uint64_count--) {
        if (*value++ != 0)
            return FALSE;
    }
    return TRUE;
}

static inline bool is_equal_uint(const uint64_t *value, size_t uint64_count, uint64_t scalar)
{
#ifdef HEDGE_DEBUG
    if (!value)
    {
        invalid_argument("value");
    }
    if (!uint64_count)
    {
        invalid_argument("uint64_count");
    }
#endif
    if (*value++ != scalar)
    {
        return false;
    }
    while (value != value + uint64_count - 1) {
        if (!value) return FALSE;
    }
    return TRUE;
}


static inline bool is_high_bit_set_uint(const uint64_t *value, size_t uint64_count)
{
#ifdef HEDGE_DEBUG
    if (!value)
    {
        invalid_argument("value");
    }
    if (!uint64_count)
    {
        invalid_argument("uint64_count");
    }
#endif
    return (value[uint64_count - 1] >> (BITS_PER_UINT64 - 1)) != 0;
}


static inline bool is_bit_set_uint(const uint64_t *value, size_t uint64_count, int bit_index)
{
#ifdef HEDGE_DEBUG
    if (!value)
    {
        invalid_argument("value");
    }
    if (!uint64_count)
    {
        invalid_argument("uint64_count");
    }
    if (bit_index < 0 ||
        (int64_t)(bit_index) >=
        (int64_t)(uint64_count) * BITS_PER_UINT64)
    {
        invalid_argument("bit_index");
    }
#endif
    int uint64_index = bit_index / BITS_PER_UINT64;
    int sub_bit_index = bit_index - uint64_index * BITS_PER_UINT64;
    return ((value[(size_t)(uint64_index)]
        >> sub_bit_index) & 1) != 0;
}

static inline void set_bit_uint(uint64_t *value, size_t uint64_count, int bit_index)
{
#ifdef HEDGE_DEBUG
    if (!value)
    {
        invalid_argument("value");
    }
    if (!uint64_count)
    {
        invalid_argument("uint64_count");
    }
    if (bit_index < 0 ||
        (int64_t)(bit_index) >=
        (int64_t)(uint64_count) * BITS_PER_UINT64)
    {
        invalid_argument("bit_index");
    }
#endif
    int uint64_index = bit_index / BITS_PER_UINT64;
    int sub_bit_index = bit_index % BITS_PER_UINT64;
    value[(size_t)(uint64_index)] |=
        (uint64_t)(1) << sub_bit_index;
}
static inline int get_significant_bit_count_uint(const uint64_t *value, size_t uint64_count)
{
#ifdef HEDGE_DEBUG
    if (!value && uint64_count) {
        invalid_argument("value");
    }
    if (!uint64_count) {
        invalid_argument("uint64_count");
    }
#endif
    if (!uint64_count) {
        return 0;
    }

    value += uint64_count - 1;
    for (; *value == 0 && uint64_count > 1; uint64_count--) {
        value--;
    }

    return (int)(uint64_count - 1) * BITS_PER_UINT64 + get_significant_bit_count(*value);
}


static inline size_t get_significant_uint64_count_uint(const uint64_t *value, size_t uint64_count)
{
#ifdef HEDGE_DEBUG
    if (!value && uint64_count)
    {
        invalid_argument("value");
    }
    if (!uint64_count)
    {
        invalid_argument("uint64_count");
    }
#endif
    value += uint64_count - 1;
    for (; uint64_count && !*value; uint64_count--)
    {
        value--;
    }

    return uint64_count;
}
   
static inline size_t get_nonzero_uint64_count_uint(const uint64_t *value, size_t uint64_count)
{
#ifdef HEDGE_DEBUG
    if (!value && uint64_count)
    {
        invalid_argument("value");
    }
    if (!uint64_count)
    {
        invalid_argument("uint64_count");
    }
#endif
    size_t nonzero_count = uint64_count;

    value += uint64_count - 1;
    for (; uint64_count; uint64_count--)
    {
        if (*value-- == 0)
        {
            nonzero_count--;
        }
    }

    return nonzero_count;
}

static inline void set_uint_uint_4(const uint64_t *value, size_t value_uint64_count,
    size_t result_uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!value && value_uint64_count)
    {
        invalid_argument("value");
    }
    if (!result && result_uint64_count)
    {
        invalid_argument("result");
    }
#endif
    if (value == result || !value_uint64_count)
    {
        // Fast path to handle self assignment.
        fill(result + value_uint64_count,
            result + result_uint64_count, (uint64_t)(0));
    }
    else
    {
        size_t min_uint64_count =
            min(value_uint64_count, result_uint64_count);
        copy_n(value, min_uint64_count, result);
        fill(result + min_uint64_count,
            result + result_uint64_count, (uint64_t)(0));
    }
}
static inline void set_uint_uint(const uint64_t *value, size_t uint64_count, uint64_t *result)
{
    set_uint_uint_4(value, uint64_count, uint64_count, result);
}

static inline int get_power_of_two(uint64_t value)
{
    if (value == 0 || (value & (value - 1)) != 0) {
        return -1;
    }
    return get_msb_index_generic(value);
}

static inline int get_power_of_two_minus_one(uint64_t value)
{
    if (value == 0xFFFFFFFFFFFFFFFF)
    {
        return BITS_PER_UINT64;
    }
    return get_power_of_two(value + 1);
}

static inline int get_power_of_two_uint(const uint64_t *operand, size_t uint64_count)
{
#ifdef HEDGE_DEBUG
    if (!operand && uint64_count)
    {
        invalid_argument("operand");
    }
#endif
    operand += uint64_count;
    int long_index = uint64_count;
    int local_result = -1;
    for (; (long_index >= 1) && (local_result == -1); long_index--)
    {
        operand--;
        local_result = get_power_of_two(*operand);
    }

    // If local_result != -1, we've found a power-of-two highest order block,
    // in which case need to check that rest are zero.
    // If local_result == -1, operand is not power of two.
    if (local_result == -1)
    {
        return -1;
    }

    int zeros = 1;
    for (int j = long_index; j >= 1; j--)
    {
        zeros &= (*--operand == 0);
    }

    return add_safe(mul_safe(zeros,
        add_safe(local_result,
            mul_safe(long_index, BITS_PER_UINT64))), zeros, -1);
}

static inline int get_power_of_two_minus_one_uint(const uint64_t *operand,
    size_t uint64_count)
{
#ifdef HEDGE_DEBUG
    if (!operand && uint64_count)
    {
        invalid_argument("operand");
    }
    if (unsigned_eq(uint64_count, INT_MAX))
    {
        invalid_argument("uint64_count");
    }
#endif
    operand += uint64_count;
    int long_index = uint64_count;
    int local_result = 0;
    for (; (long_index >= 1) && (local_result == 0); long_index--)
    {
        operand--;
        local_result = get_power_of_two_minus_one(*operand);
    }

    // If local_result != -1, we've found a power-of-two-minus-one highest
    // order block, in which case need to check that rest are ~0.
    // If local_result == -1, operand is not power of two minus one.
    if (local_result == -1)
    {
        return -1;
    }

    int ones = 1;
    for (int j = long_index; j >= 1; j--)
    {
        ones &= (~*--operand == 0);
    }

    return add_safe(mul_safe(ones,
        add_safe(local_result,
            mul_safe(long_index, BITS_PER_UINT64))), ones, -1);
}

static inline void filter_highbits_uint(uint64_t *operand,
            size_t uint64_count, int bit_count)
{
    size_t bits_per_uint64_sz = BITS_PER_UINT64;
#ifdef HEDGE_DEBUG
    if (!operand && uint64_count)
    {
        invalid_argument("operand");
    }
    if (bit_count < 0 || unsigned_gt(bit_count,
        mul_safe(uint64_count, bits_per_uint64_sz)))
    {
        invalid_argument("bit_count");
    }
#endif
    if (unsigned_eq(bit_count, mul_safe(uint64_count, bits_per_uint64_sz)))
    {
        return;
    }
    int uint64_index = bit_count / BITS_PER_UINT64;
    int subbit_index = bit_count - uint64_index * BITS_PER_UINT64;
    operand += uint64_index;
    *operand++ &= ((uint64_t)(1) << subbit_index) - 1;
    for (int long_index = uint64_index + 1;
        unsigned_lt(long_index, uint64_count); long_index++)
    {
        *operand++ = 0;
    }
}

static inline const uint64_t* duplicate_uint_if_needed(const uint64_t *input, size_t uint64_count,
    size_t new_uint64_count, bool force)
{
#ifdef HEDGE_DEBUG
    if (!input && uint64_count)
    {
        invalid_argument("uint");
    }
#endif
    if (!force && uint64_count >= new_uint64_count)
    {
        return input;
    }

    uint64_t* allocation = hedge_malloc(sizeof(uint64_t) * new_uint64_count);
    set_uint_uint_4(input, uint64_count, new_uint64_count, allocation);
    return allocation;
}


static inline int compare_uint_uint_3(const uint64_t *operand1, const uint64_t *operand2,
    size_t uint64_count)
{
#ifdef HEDGE_DEBUG
    if (!operand1 && uint64_count)
    {
        invalid_argument("operand1");
    }
    if (!operand2 && uint64_count)
    {
        invalid_argument("operand2");
    }
#endif
    int result = 0;
    operand1 += uint64_count - 1;
    operand2 += uint64_count - 1;

    for (; (result == 0) && uint64_count--; operand1--, operand2--)
    {
        result = (*operand1 > *operand2) - (*operand1 < *operand2);
    }
    return result;
}


static inline int compare_uint_uint(const uint64_t *operand1, size_t operand1_uint64_count, 
    const uint64_t *operand2, size_t operand2_uint64_count)
{
#ifdef HEDGE_DEBUG
    if (!operand1 && operand1_uint64_count)
    {
        invalid_argument("operand1");
    }
    if (!operand2 && operand2_uint64_count)
    {
        invalid_argument("operand2");
    }
#endif
    int result = 0;
    operand1 += operand1_uint64_count - 1;
    operand2 += operand2_uint64_count - 1;

    size_t min_uint64_count =
        min(operand1_uint64_count, operand2_uint64_count);

    operand1_uint64_count -= min_uint64_count;
    for (; (result == 0) && operand1_uint64_count--; operand1--)
    {
        result = (*operand1 > 0);
    }

    operand2_uint64_count -= min_uint64_count;
    for (; (result == 0) && operand2_uint64_count--; operand2--)
    {
        result = -(*operand2 > 0);
    }

    for (; (result == 0) && min_uint64_count--; operand1--, operand2--)
    {
        result = (*operand1 > *operand2) - (*operand1 < *operand2);
    }
    return result;
}

static inline bool is_greater_than_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count)
{
    return compare_uint_uint_3(operand1, operand2, uint64_count) > 0;
}

static inline bool is_greater_than_uint_uint_4(const uint64_t *operand1,
    size_t operand1_uint64_count, const uint64_t *operand2,
    size_t operand2_uint64_count)
{
    return compare_uint_uint(operand1, operand1_uint64_count, operand2,
        operand2_uint64_count) > 0;
}

static inline bool is_greater_than_or_equal_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count)
{
    return compare_uint_uint_3(operand1, operand2, uint64_count) >= 0;
}

static inline bool is_greater_than_or_equal_uint_uint_4(
    const uint64_t *operand1, size_t operand1_uint64_count,
    const uint64_t *operand2, size_t operand2_uint64_count)
{
    return compare_uint_uint(operand1, operand1_uint64_count, operand2,
        operand2_uint64_count) >= 0;
}

static inline bool is_less_than_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count)
{
    return compare_uint_uint_3(operand1, operand2, uint64_count) < 0;
}

static inline bool is_less_than_uint_uint_4(const uint64_t *operand1,
    size_t operand1_uint64_count, const uint64_t *operand2,
    size_t operand2_uint64_count)
{
    return compare_uint_uint(operand1, operand1_uint64_count, operand2,
        operand2_uint64_count) < 0;
}

static inline bool is_less_than_or_equal_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count)
{
    return compare_uint_uint_3(operand1, operand2, uint64_count) <= 0;
}

static inline bool is_less_than_or_equal_uint_uint_4(const uint64_t *operand1,
    size_t operand1_uint64_count, const uint64_t *operand2,
    size_t operand2_uint64_count)
{
    return compare_uint_uint(operand1, operand1_uint64_count, operand2,
        operand2_uint64_count) <= 0;
}

static inline bool is_equal_uint_uint_3(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count)
{
    return compare_uint_uint_3(operand1, operand2, uint64_count) == 0;
}

static inline bool is_equal_uint_uint(const uint64_t *operand1,
    size_t operand1_uint64_count, const uint64_t *operand2,
    size_t operand2_uint64_count)
{
    return compare_uint_uint(operand1, operand1_uint64_count, operand2,
        operand2_uint64_count) == 0;
}

static inline bool is_not_equal_uint_uint(const uint64_t *operand1,
    const uint64_t *operand2, size_t uint64_count)
{
    return compare_uint_uint_3(operand1, operand2, uint64_count) != 0;
}

static inline bool is_not_equal_uint_uint_4(const uint64_t *operand1,
    size_t operand1_uint64_count, const uint64_t *operand2,
    size_t operand2_uint64_count)
{
    return compare_uint_uint(operand1, operand1_uint64_count, operand2,
        operand2_uint64_count) != 0;
}

static inline uint64_t hamming_weight(uint64_t value)
{
    uint64_t res = 0;
    while (value)
    {
        res++;
        value &= value - 1;
    }
    return res;
}

static inline uint64_t hamming_weight_split(uint64_t value)
{
    uint64_t hwx = hamming_weight(value);
    uint64_t target = (hwx + 1) >> 1;
    uint64_t now = 0;
    uint64_t result = 0;

    for (int i = 0; i < BITS_PER_UINT64; i++)
    {
        uint64_t xbit = value & 1;
        value = value >> 1;
        now += xbit;
        result += (xbit << i);

        if (now >= target)
        {
            break;
        }
    }
    return result;
}

#endif /* __UINTCORE_H__ */