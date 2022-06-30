#ifndef __NUMTH_H__
#define __NUMTH_H__

#include "lib/types.h"
#include "lib/vector.h"
#include "lib/pair.h"
#include "math/smallmodulus.h"

struct tuple {
    uint64_t val[3];
};

uint64_t gcd(uint64_t x, uint64_t y);
void xgcd(uint64_t x, uint64_t y, struct tuple* result);
vector(uint64_t)* conjugate_classes(uint64_t modulus, uint64_t subgroup_generator);
vector(uint64_t)* multiplicative_orders(
    vector(uint64_t)* conjugate_classes, uint64_t modulus);
void babystep_giantstep(uint64_t modulus,
    vector(uint64_t)* baby_steps, vector(uint64_t)* giant_steps);
pair(size_t, size_t) decompose_babystep_giantstep(
    uint64_t modulus, uint64_t input,
    const vector(uint64_t)* baby_steps,
    const vector(uint64_t)* giant_steps);
bool is_prime(Zmodulus* modulus, size_t num_rounds);
vector(Zmodulus)* get_primes(size_t ntt_size, int bit_size, size_t count);
bool is_prime_number(uint64_t number);

bool try_mod_inverse(uint64_t value, uint64_t modulus, uint64_t *result);

#endif /* __NUMTH_H__ */