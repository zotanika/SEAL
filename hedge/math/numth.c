#include "math/smallmodulus.h"
#include "lib/vector.h"
#include "math/rng.h"
#include "math/numth.h"
#include "math/uintcore.h"
#include "math/uintarithsmallmod.h"

uint64_t gcd(uint64_t x, uint64_t y)
{
#ifdef HEDGE_DEBUG
    if (x == 0){
        invalid_argument("x cannot be zero");
    }

    if (y == 0){
        invalid_argument("y cannot be zero");
    }
#endif
    if (x < y){
        return gcd(y, x);
    } else if (y == 0){
        return x;
    } else {
        uint64_t f = x % y;
        if (f == 0) {
            return y;
        } else {
            return gcd(y, f);
        }
    }
}

void xgcd(uint64_t x, uint64_t y, struct tuple* result)
{
    /* Extended GCD:
    Returns (gcd, x, y) where gcd is the greatest common divisor of a and b.
    The numbers x, y are such that gcd = ax + by.
    */
#ifdef HEDGE_DEBUG
    if (x == 0) {
        invalid_argument("x cannot be zero");
    }
    if (y == 0) {
        invalid_argument("y cannot be zero");
    }
#endif
    int64_t prev_a = 1;
    int64_t a = 0;
    int64_t prev_b = 0;
    int64_t b = 1;

    while (y != 0) {
        int64_t q = (int64_t)(x / y);
        int64_t temp = (int64_t)(x % y);
        x = y;
        y = (uint64_t)(temp);

        temp = a;
        a = sub_safe_signed(prev_a, mul_safe_signed(q, a));
//        a = prev_a - (q * a);
        prev_a = temp;

        temp = b;
        b = sub_safe_signed(prev_b, mul_safe_signed(q, b));
//        b = prev_b - (q * b);
        prev_b = temp;
    }
    result->val[0] = x;
    result->val[1] = prev_a;
    result->val[2] = prev_b;
    return;
}

vector(uint64_t)* conjugate_classes(uint64_t modulus, uint64_t subgroup_generator)
{
    uint64_t i;

    if(!product_fits_in(modulus, subgroup_generator)) {
        return NULL;
    }

    vector(uint64_t)* classes = new_vector(uint64_t, modulus);
    
    for (i = 0; i < modulus; i++) {
        if (gcd(i, modulus) > 1) {
            classes->push_back(classes, 0);
        } else {
            classes->push_back(classes, i);
        }
    }

    for (i = 0; i < modulus; i++) {
        if (classes->value_at(classes, i) == 0) {
            continue;
        }
        if (classes->value_at(classes, i) < i) {
            // i is not a pivot, updated its pivot
            *classes->at(classes, i) = classes->value_at(classes, classes->value_at(classes, i));
            continue;
        }

        // If i is a pivot, update other pivots to point to it
        uint64_t j = (i * subgroup_generator) % modulus;
        while (classes->value_at(classes, j) != i)
        {
            // Merge the equivalence classes of j and i
            // Note: if classes[j] != j then classes[j] will be updated later,
            // when we get to i = j and use the code for "i not pivot".
            *classes->at(classes, classes->value_at(classes, j)) = i;
            j = (j * subgroup_generator) % modulus;
        }
    }
    return classes;
}

vector(uint64_t)* multiplicative_orders(
    vector(uint64_t)* conjugate_classes, uint64_t modulus)
{
    uint64_t i;

    if (!product_fits_in(modulus, modulus))
    {
        return NULL; //inputs too large
    }

    vector(uint64_t)* orders = new_vector(uint64_t, modulus);
    orders->push_back(orders, 0);
    orders->push_back(orders, 1);

    for (i = 2; i < modulus; i++) {
        if (orders->value_at(conjugate_classes, i) <= 1) {
            orders->push_back(orders, orders->value_at(conjugate_classes, i));
            continue;
        }
        
        if (orders->value_at(conjugate_classes, i) < i) {
            orders->push_back(orders, orders->value_at(orders, orders->value_at(conjugate_classes, 1)));
            continue;
        }

        uint64_t j = (i * i) % modulus;
        uint64_t order = 2;
        while (orders->value_at(conjugate_classes, j) != 1)
        {
            j = (j * i) % modulus;
            order++;
        }
        orders->push_back(orders, order);
    }
    return orders;
}


void babystep_giantstep(uint64_t modulus,
    vector(uint64_t)* baby_steps, vector(uint64_t)* giant_steps)
{
    uint64_t i,j;
    int exponent = get_power_of_two(modulus);
    if (exponent < 0)
    {
        return; //throw invalid_argument("modulus must be a power of 2");
    }

    // Compute square root of modulus (k stores the baby steps)
    uint64_t k = (uint64_t)(1) << (exponent / 2);
    uint64_t l = modulus / k;

    baby_steps->clear(baby_steps);
    giant_steps->clear(giant_steps);

    uint64_t m = mul_safe(modulus, 2);
    uint64_t g = 3; // the generator
    uint64_t kprime = k >> 1;
    uint64_t value = 1;
    for (i = 0; i < kprime; i++)
    {
        baby_steps->push_back(baby_steps, value);
        baby_steps->push_back(baby_steps, (m - value));
        value = mul_safe(value, g) % m;
    }

    // now value should equal to g**kprime
    uint64_t value2 = value;
    for (j = 0; j < l; j++)
    {
        giant_steps->push_back(giant_steps, value2);
        value2 = mul_safe(value2, value) % m;
    }
}

pair(size_t, size_t) decompose_babystep_giantstep(
    uint64_t modulus, uint64_t input,
    const vector(uint64_t)* baby_steps,
    const vector(uint64_t)* giant_steps)
{
    size_t i, j;
    pair(size_t, size_t) ret;
    for (i = 0; i < giant_steps->size; i++)
    {
        uint64_t gs = giant_steps->value_at((vector(uint64_t)*)giant_steps, i);
        for (j = 0; j < baby_steps->size; j++)
        {
            uint64_t bs = baby_steps->value_at((vector(uint64_t)*)baby_steps, j);
            if (mul_safe(gs, bs) % modulus == input)
            {
                ret.first = i;
                ret.second = j;
                return ret;
            }
        }
    }
    logic_error("failed to decompose input");
    ret.first = 0;
    ret.second = 0;
    return ret;
}


bool is_prime(Zmodulus* modulus, size_t num_rounds)
{
    uint64_t value = modulus->value;
    log_verbose("is_prime(%lu) value=%lu\n",num_rounds, value);
    // First check the simplest cases.
    if (value < 2)
    {
        return false;
    }
    if (2 == value)
    {
        return true;
    }
    if (0 == (value & 0x1))
    {
        return false;
    }
    if (3 == value)
    {
        return true;
    }
    if (0 == (value % 3))
    {
        return false;
    }
    if (5 == value)
    {
        return true;
    }
    if (0 == (value % 5))
    {
        return false;
    }
    if (7 == value)
    {
        return true;
    }
    if (0 == (value % 7))
    {
        return false;
    }
    if (11 == value)
    {
        return true;
    }
    if (0 == (value % 11))
    {
        return false;
    }
    if (13 == value)
    {
        return true;
    }
    if (0 == (value % 13))
    {
        return false;
    }

    // Second, Miller-Rabin test.
    // Find r and odd d that satisfy value = 2^r * d + 1.
    uint64_t d = value - 1;
    uint64_t r = 0;
    while (0 == (d & 0x1))
    {
        d >>= 1;
        r++;
    }
    if (r == 0)
    {
        return false;
    }

    // 1) Pick a = 2, check a^(value - 1).
    // 2) Pick a randomly from [3, value - 1], check a^(value - 1).
    // 3) Repeat 2) for another num_rounds - 2 times.

    struct rng rng;
    rng_init_uniform(&rng, rng_get_seed(), 3, value - 1);
    log_debug("r = %ld\n", r);
    for (size_t i = 0; i < num_rounds; i++)
    {
        uint64_t random_number = (uint64_t)rng_get(&rng);
        uint64_t a = i ? random_number : 2;
        log_debug("random_number=%ld\n", random_number);
        log_debug("%ld:random int from 3 to %ld : %ld\n", i, value-1, a);
        uint64_t x = exponentiate_uint_smallmod(a, d, modulus);
        log_debug("exponentiate_uint_smallmod(%lu, %lu, mod=%lu)=%lu\n", a, d, modulus->value, x);
        if (x == 1 || x == value - 1)
        {
            continue;
        }
        uint64_t count = 0;
        do
        {
            x = multiply_uint_uint_smallmod(x, x, modulus);
            log_debug("@@ x=%lu, modulus=%lu\n", x, modulus->value);
            count++;
        } while (x != value - 1 && count < r - 1);
        if (x != value - 1)
        {
            return false;
        }
    }
    return true;
}

bool is_prime_number(uint64_t number)
{
    Zmodulus mod = *set_zmodulus_with(&mod, number);
    return mod.is_prime;
}

vector(Zmodulus)* get_primes(size_t ntt_size, int bit_size, size_t count)
{
    if (!count)
    {
        invalid_argument("count must be positive");
    }
    if (!ntt_size)
    {
        invalid_argument("ntt_size must be positive");
    }
    if (bit_size >= 63 || bit_size <= 1)
    {
        invalid_argument("bit_size is invalid");
    }

    vector(Zmodulus)* destination = new_vector(Zmodulus, count);
    uint64_t factor = mul_safe((uint64_t)2, (uint64_t)(ntt_size));

    // Start with 2^bit_size - 2 * ntt_size + 1
    uint64_t value = (uint64_t)(0x1) << bit_size;
    log_debug("get_primes(%lu, %d, %lu) factor=%lu, value=%lu\n",
        ntt_size, bit_size, count, factor, value);
    value = sub_safe(value, factor) + 1;

    uint64_t lower_bound = (uint64_t)(0x1) << (bit_size - 1);
    while (count > 0 && value > lower_bound)
    {
        Zmodulus new_mod = *set_zmodulus_with(&new_mod, value);
        if (new_mod.is_prime)
        {
            destination->emplace_back(destination, &new_mod);
            count--;
            log_debug("prime number found : %lu\n", value);
        }
        value -= factor;
    }
    if (count > 0)
    {
        logic_error("failed to find enough qualifying primes");
    }
    print_array_info(destination, count);

    return destination;
}

bool try_mod_inverse(uint64_t value, uint64_t modulus, uint64_t *result)
{
#ifdef HEDGE_DEBUG
    if (value == 0)
    {
        invalid_argument("value cannot be zero");
    }
    if (modulus <= 1)
    {
        invalid_argument("modulus must be at least 2");
    }
#endif
    struct tuple gcd_tuple;
    xgcd(value, modulus, &gcd_tuple);
    if ((int64_t)gcd_tuple.val[0] != 1)
    {
        return false;
    }
    else if ((int64_t)gcd_tuple.val[1] < 0)
    {
        *result = (uint64_t)((int64_t)gcd_tuple.val[1] + (int64_t)modulus);
        return true;
    }
    else
    {
        *result = (uint64_t)(gcd_tuple.val[1]);
        return true;
    }
}
