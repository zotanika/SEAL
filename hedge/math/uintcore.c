#include "math/uintcore.h"
#include "math/uintarith.h"
#include <limits.h>
#include "lib/types.h"
#include "lib/algorithm.h"
#include "lib/common.h"
#include "hedge_malloc.h"
#include "defines.h"

char* uint_to_hex_string(const uint64_t* value, size_t uint64_count)
{
#ifdef HEDGE_DEBUG
    if (uint64_count && !value){
        invalid_argument("value");
    }
#endif
    // Start with a string with a zero for each nibble in the array.
    size_t num_nibbles =
        mul_safe(uint64_count, NIBBLES_PER_UINT64);
    char* output = hedge_zalloc(num_nibbles + 1);
    output[0] = '0';

    // Iterate through each uint64 in array and set string with correct nibbles in hex.
    size_t nibble_index = num_nibbles;
    size_t leftmost_non_zero_pos = num_nibbles;
    for (size_t i = 0; i < uint64_count; i++)
    {
        uint64_t part = *value++;

        // Iterate through each nibble in the current uint64.
        for (size_t j = 0; j < NIBBLES_PER_UINT64; j++)
        {
            size_t nibble = (size_t)(part & (uint64_t)(0x0F));
            size_t pos = --nibble_index;
            if (nibble != 0)
            {
                // If nibble is not zero, then update string and save this pos to determine
                // number of leading zeros.
                output[pos] = nibble_to_upper_hex((int)(nibble));
                leftmost_non_zero_pos = pos;
            }
            part >>= 4;
        }
    }

    // Trim string to remove leading zeros.
    //output.erase(0, leftmost_non_zero_pos);

    return output;
}

char* uint_to_dec_string(const uint64_t* value, size_t uint64_count)
{
    char* output = hedge_zalloc(uint64_count * 2);
    output[0] = '0';
#ifdef HEDGE_DEBUG
    if (uint64_count && !value)
    {
        invalid_argument("value");
    }
#endif
    if (!uint64_count)
    {
        return output;
    }
    uint64_t *remainder = allocate_uint(uint64_count);
    uint64_t *quotient = allocate_uint(uint64_count);
    uint64_t *base = allocate_uint(uint64_count);
    uint64_t *remainderptr = remainder;
    uint64_t *quotientptr = quotient;
    uint64_t *baseptr = base;
    size_t idx = 0;
    set_uint(10, uint64_count, baseptr);
    set_uint_uint(value, uint64_count, remainderptr);

    while (!is_zero_uint(remainderptr, uint64_count))
    {
        divide_uint_uint_inplace(remainderptr, baseptr, uint64_count,
            quotientptr);
        char digit = (char)(remainderptr[0] + (uint64_t)('0'));
        output[idx++] = digit;
        swap(&remainderptr, &quotientptr);
    }
    reverse(output, output + idx);

    return output;
}


void hex_string_to_uint(char* hex_string, int char_count, size_t uint64_count, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (!hex_string && char_count > 0)
    {
        invalid_argument("hex_string");
    }
    if (uint64_count && !result)
    {
        invalid_argument("result");
    }
    if (unsigned_gt(get_hex_string_bit_count(hex_string, char_count),
        mul_safe(uint64_count, BITS_PER_UINT64)))
    {
        invalid_argument("hex_string");
    }
#endif
    const char *hex_string_ptr = hex_string + char_count;
    for (size_t uint64_index = 0;
        uint64_index < uint64_count; uint64_index++)
    {
        uint64_t value = 0;
        for (int bit_index = 0; bit_index < BITS_PER_UINT64;
            bit_index += BITS_PER_NIBBLE)
        {
            if (hex_string_ptr == hex_string)
            {
                break;
            }
            char hex = *--hex_string_ptr;
            int nibble = hex_to_nibble(hex);
            if (nibble == -1)
            {
                invalid_argument("hex_value");
            }
            value |= (uint64_t)nibble << bit_index;
        }
        result[uint64_index] = value;
    }
}

