#ifndef __POLYCORE_H__
#define __POLYCORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lib/types.h"
#include <limits.h>
#include <string.h>
#include "lib/common.h"
#include "math/uintcore.h"

static inline char *poly_to_hex_string(
        const uint64_t *value,
        size_t coeff_count,
        size_t coeff_uint64_count)
{
#ifdef HEDGE_DEBUG
    assert(!value && coeff_uint64_count && coeff_count);
#endif
    char *result;
    char *resptr;
    char *tmpptr = NULL;
    bool empty = true;

    value += mul_safe(coeff_count - 1, coeff_uint64_count);

    /* uint64_t can contain number upto 10^20 */
    result = (char *)hedge_zalloc(sizeof(char) * (*value) * (20 + 3));
    if (!result)
        return NULL;
    else
        resptr = result;

    while (coeff_count--) {
        if (is_zero_uint(value, coeff_uint64_count)) {
            value -= coeff_uint64_count;
            continue;
        }
        if (!empty) {
            strncat(resptr, " + ", 3);
            resptr += 3;
        }
        /* uint_to_hex_string itself allocates the memory for hex string inside */
        tmpptr = uint_to_hex_string(value, coeff_uint64_count);
        strncat(resptr, (const char *)tmpptr, strlen(tmpptr));
        resptr += strlen(tmpptr);
        if (coeff_count) {
            strncat(resptr, "x^", 2);
            resptr += 2;
        }
        empty = false;
        value -= coeff_uint64_count;
    }
    if (empty) {
        strncat(resptr, "0", 1);
        resptr++;
    }
    return result;
}

static inline char *poly_to_dec_string(
        const uint64_t *value,
        size_t coeff_count,
        size_t coeff_uint64_count)
{
#ifdef HEDGE_DEBUG
    assert(!value && coeff_uint64_count && coeff_count);
#endif
    char *result;
    char *resptr;
    char *tmpptr;
    bool empty = true;
    value += coeff_count - 1;

    /* uint64_t can contain number upto 10^20 */
    result = (char *)hedge_zalloc(sizeof(char) * (*value) * (20 + 3));
    if (!result)
        return NULL;
    else
        resptr = result;

    while (coeff_count--) {
        if (is_zero_uint(value, coeff_uint64_count)) {
            value -= coeff_uint64_count;
            continue;
        }
        if (!empty) {
            strncat(resptr, " + ", 3);
            resptr += 3;
        }
        /* uint_to_hex_string itself allocates the memory for dec string inside */
        tmpptr = uint_to_dec_string(value, coeff_uint64_count);
        strncat(resptr, (const char *)tmpptr, strlen(tmpptr));
        resptr += strlen(tmpptr);
        if (coeff_count) {
            strncat(resptr, "x^", 2);
            resptr += 2;
        }
        empty = false;
        value -= coeff_uint64_count;
    }
    if (empty) {
        strncat(resptr, "0", 1);
        resptr++;
    }
    return result;
}

static inline void *allocate_poly(size_t coeff_count, size_t coeff_uint64_count)
{
    return (void *)allocate_uint(mul_safe(coeff_count, coeff_uint64_count));
}

static inline void set_zero_poly(size_t coeff_count, size_t coeff_uint64_count, uint64_t* result)
{
#ifdef HEDGE_DEBUG
    assert(!result && coeff_count && coeff_uint64_count);
#endif
    set_zero_uint(mul_safe(coeff_count, coeff_uint64_count), result);
}

static inline void *allocate_zero_poly(size_t coeff_count, size_t coeff_uint64_count)
{
    return (void *)allocate_zero_uint(mul_safe(coeff_count, coeff_uint64_count));
}

static inline const uint64_t *get_poly_coeff(const uint64_t *poly, size_t coeff_index, size_t coeff_uint64_count)
{
#ifdef HEDGE_DEBUG
    assert(!poly);
#endif
    return poly + mul_safe(coeff_index, coeff_uint64_count);
}

static inline bool is_zero_poly(
        const uint64_t *poly,
        size_t coeff_count,
        size_t coeff_uint64_count)
{
#ifdef HEDGE_DEBUG
    assert(poly && coeff_count && coeff_uint64_count);
#endif
    return is_zero_uint(poly, mul_safe(coeff_count, coeff_uint64_count));
}

static inline bool is_equal_poly_poly(
        const uint64_t *operand1,
        const uint64_t *operand2,
        size_t coeff_count,
        size_t coeff_uint64_count)
{
#ifdef HEDGE_DEBUG
    assert(!operand1 && coeff_count && coeff_uint64_count);
    assert(!operand2 && coeff_count && coeff_uint64_count);
#endif
    return is_equal_uint_uint(operand1, mul_safe(coeff_count, coeff_uint64_count), operand2, mul_safe(coeff_count, coeff_uint64_count));
}

static inline void set_poly_poly(
        const uint64_t *poly,
        size_t poly_coeff_count,
        size_t poly_coeff_uint64_count,
        size_t result_coeff_count,
        size_t result_coeff_uint64_count,
        uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(!poly && poly_coeff_count && poly_coeff_uint64_count);
    assert(!result && result_coeff_count && result_coeff_uint64_count);
#endif
    size_t i;
    size_t min_coeff_count = min(poly_coeff_count, result_coeff_count);

    if (!result_coeff_uint64_count || !result_coeff_count)
        return;

    for (i = 0; i < min_coeff_count;
         i++, poly += poly_coeff_uint64_count, result += result_coeff_uint64_count) {
        set_uint_uint_4(poly, poly_coeff_uint64_count, result_coeff_uint64_count, result);
    }
    set_zero_uint(mul_safe(result_coeff_count - min_coeff_count,
                           result_coeff_uint64_count), result);
}

static inline void set_poly_poly_simple(
        const uint64_t *poly,
        size_t coeff_count,
        size_t coeff_uint64_count,
        uint64_t *result)
{
#ifdef HEDGE_DEBUG
    assert(poly && coeff_count && coeff_uint64_count);
    assert(result && coeff_count && coeff_uint64_count);
#endif
    set_uint_uint_4(poly, mul_safe(coeff_count, coeff_uint64_count), mul_safe(coeff_count, coeff_uint64_count), result);
}


static inline bool is_one_zero_one_poly(
        const uint64_t *poly,
        size_t coeff_count,
        size_t coeff_uint64_count)
{
#ifdef HEDGE_DEBUG
    assert(poly && coeff_count && coeff_uint64_count);
#endif
    if (coeff_count == 0 || coeff_uint64_count == 0)
        return false;

    if (!is_equal_uint(get_poly_coeff(poly, 0, coeff_uint64_count),
                coeff_uint64_count, 1))
        return false;

    if (!is_equal_uint(get_poly_coeff(poly, coeff_count - 1, coeff_uint64_count),
                coeff_uint64_count, 1))
        return false;

    return true;
}

static inline size_t get_significant_coeff_count_poly(
        const uint64_t *poly,
        size_t coeff_count,
        size_t coeff_uint64_count)
{
#ifdef HEDGE_DEBUG
    assert(poly && coeff_count && coeff_uint64_count);
#endif
    size_t i;

    if (coeff_count == 0)
        return 0;

    poly += mul_safe(coeff_count - 1, coeff_uint64_count);
    for (size_t i = coeff_count; i; i--) {
        if (!is_zero_uint(poly, coeff_uint64_count))
            return i;
        poly -= coeff_uint64_count;
    }
    return 0;
}

static inline void *duplicate_poly_if_needed(
        const uint64_t *poly,
        size_t coeff_count,
        size_t coeff_uint64_count,
        size_t new_coeff_count,
        size_t new_coeff_uint64_count,
        bool force)
{
#ifdef HEDGE_DEBUG
    assert(poly && coeff_count && coeff_uint64_count);
#endif
    uint64_t *new_poly;

    if (!force &&
        coeff_count >= new_coeff_count && coeff_uint64_count == new_coeff_uint64_count) {
        return (void *)(poly);
    }

    new_poly = allocate_poly(new_coeff_count, new_coeff_uint64_count);
    set_poly_poly(poly, coeff_count, coeff_uint64_count, new_coeff_count,
            new_coeff_uint64_count, new_poly);
    return (void *)new_poly;
}

static inline bool are_poly_coefficients_less_than_ex(
        const uint64_t *poly,
        size_t coeff_count,
        size_t coeff_uint64_count,
        const uint64_t *compare,
        size_t compare_uint64_count)
{
#ifdef HEDGE_DEBUG
    assert(poly && coeff_count && coeff_uint64_count);
    assert(compare && coeff_count && coeff_uint64_count);
#endif
    if (!coeff_count)           return true;
    if (!compare_uint64_count)  return false;
    if (!coeff_uint64_count)    return true;

    for (; coeff_count--; poly += coeff_uint64_count) {
        if (compare_uint_uint(
                    poly, coeff_uint64_count,
                    compare, compare_uint64_count) >= 0) {
            return false;
        }
    }
    return true;
}

static inline bool are_poly_coefficients_less_than(
        const uint64_t *poly,
        size_t coeff_count,
        uint64_t compare)
{
#ifdef HEDGE_DEBUG
    assert(!poly && coeff_count);
#endif
    if (!coeff_count)
        return true;

    for (; coeff_count--; poly++) {
        if (*poly >= compare) {
            return false;
        }
    }
    return true;
}

#ifdef __cplusplus
}
#endif

#endif /* __POLYCORE_H__ */