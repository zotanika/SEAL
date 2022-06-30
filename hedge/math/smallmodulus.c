// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "math/smallmodulus.h"
#include "math/uintarith.h"
#include "math/uintarithsmallmod.h"
#include "lib/common.h"
#include "math/numth.h"

Zmodulus* set_zmodulus_with(Zmodulus* container, uint64_t value)
{
    if (value == 0)
    {
        // Zero settings
        container->bit_count = 0;
        container->uint64_count = 1;
        container->value = 0;
        memset(container->const_ratio, sizeof(container->const_ratio), 0);
        container->is_prime = false;
    }
    else if ((value >> 62 != 0) || (value == (uint64_t)(0x4000000000000000)) ||
        (value == 1))
    {
        log_error("value=%ld\n", value);
        invalid_argument("value can be at most 62 bits and cannot be 1");
    }
    else
    {
        // All normal, compute const_ratio and set everything
        container->value = value;
        container->bit_count = get_significant_bit_count(container->value);

        // Compute Barrett ratios for 64-bit words (barrett_reduce_128)
        uint64_t numerator[3] = { 0, 0, 1 };
        uint64_t quotient[3] = { 0, 0, 0 };

        // Use a special method to avoid using memory pool
        divide_uint192_uint64_inplace(numerator, container->value, quotient);

        container->const_ratio[0] = quotient[0];
        container->const_ratio[1] = quotient[1];

        // We store also the remainder
        container->const_ratio[2] = numerator[0];

        container->uint64_count = 1;

        log_debug("@@ value=%lu bit_count=%lu\n", container->value, container->bit_count);

        log_debug("@@ numerator=%lu %lu %lu\n", numerator[0], numerator[1], numerator[2]);
        log_debug("@@ quotient=%lu %lu %lu\n", quotient[0], quotient[1], quotient[2]);
        
        // Set the primality flag
        container->is_prime = is_prime(container, 40);
    }
    return container;
}

static void assign(Zmodulus* self, Zmodulus* src)
{
    self->value = src->value;
    self->uint64_count = src->uint64_count;
    self->bit_count = src->bit_count;
    self->is_prime = src->is_prime;
    memcpy(self->const_ratio, src->const_ratio, sizeof(uint64_t) * 3);
}

DECLARE_VECTOR_BODY(Zmodulus);

DECLARE_UNORDERED_MAP_BODY(int, vector_Zmodulus);

Zmodulus* dup_Zmodulus(Zmodulus* src)
{
    Zmodulus* new = hedge_malloc(sizeof(Zmodulus));
    memcpy(new, src, sizeof(Zmodulus));
    return new;
}

Zmodulus* new_Zmodulus(uint64_t val)
{
    Zmodulus* p = hedge_malloc(sizeof(Zmodulus));

    // create creator function body here
    p->value = val;
    memset(p->const_ratio, sizeof(p->const_ratio), 0);
    p->uint64_count = 0;
    p->bit_count = 0;
    p->is_prime = false;

    set_zmodulus_with(p, val);
    return p;
}

void del_Zmodulus(Zmodulus* p)
{
    hedge_free(p);
}
