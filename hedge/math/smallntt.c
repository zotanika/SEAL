#include <assert.h>
#include "math/smallntt.h"
#include "math/polyarith.h"
#include "math/uintarith.h"
#include "math/smallmodulus.h"
#include "math/uintarithsmallmod.h"
#include "defines.h"
#include "hedge_malloc.h"
#include "math/numth.h"
/**
 * This function computes in-place the negacyclic NTT. The input is
 * a polynomial a of degree n in R_q, where n is assumed to be a power of
 * 2 and q is a prime such that q = 1 (mod 2n).
 * 
 * The output is a vector A such that the following hold:
 * A[j] =  a(psi**(2*bit_reverse(j) + 1)), 0 <= j < n.
 *
 * For details, see Michael Naehrig and Patrick Longa.
 */
void ntt_negacyclic_harvey_lazy(uint64_t *operand, const SmallNTTTables *tables)
{
    uint64_t modulus = tables->modulus->value;
    uint64_t two_times_modulus = modulus << 1;

    size_t n = (size_t)(1 << tables->coeff_count_power);
    size_t t = n >> 1;

    log_trace("ntt_negacyclic_harvey_lazy: modulus=%lu n=%zu t=%zu\n",modulus, n, t);
    for (size_t m = 1; m < n; m <<= 1) {
        if (t >= 4) {
            for (size_t i = 0; i < m; i++) {
                size_t j1 = 2 * i * t;
                size_t j2 = j1 + t;

                const uint64_t W = tables->get_from_root_powers((SmallNTTTables *)tables, m + i);
                const uint64_t Wprime = tables->get_from_scaled_root_powers((SmallNTTTables *)tables, m + i);

                uint64_t *X = operand + j1;
                uint64_t *Y = X + t;
                uint64_t currX;
                uint64_t Q;

                for (size_t j = j1; j < j2; j += 4) {
                    currX = *X - (two_times_modulus & (uint64_t)(-1 * (int64_t)(*X >= two_times_modulus)));
                    multiply_uint64_hw64(Wprime, *Y, &Q);
                    Q = *Y * W - Q * modulus;
                    *X++ = currX + Q;
                    *Y++ = currX + (two_times_modulus - Q);

                    currX = *X - (two_times_modulus & (uint64_t)(-1 * (int64_t)(*X >= two_times_modulus)));
                    multiply_uint64_hw64(Wprime, *Y, &Q);
                    Q = *Y * W - Q * modulus;
                    *X++ = currX + Q;
                    *Y++ = currX + (two_times_modulus - Q);

                    currX = *X - (two_times_modulus & (uint64_t)(-1 * (int64_t)(*X >= two_times_modulus)));
                    multiply_uint64_hw64(Wprime, *Y, &Q);
                    Q = *Y * W - Q * modulus;
                    *X++ = currX + Q;
                    *Y++ = currX + (two_times_modulus - Q);

                    currX = *X - (two_times_modulus & (uint64_t)(-1 * (int64_t)(*X >= two_times_modulus)));
                    multiply_uint64_hw64(Wprime, *Y, &Q);
                    Q = *Y * W - Q * modulus;
                    *X++ = currX + Q;
                    *Y++ = currX + (two_times_modulus - Q);
                }
            }
        } else {
            for (size_t i = 0; i < m; i++) {
                size_t j1 = 2 * i * t;
                size_t j2 = j1 + t;

                const uint64_t W = tables->get_from_root_powers((SmallNTTTables *)tables, m + i);
                const uint64_t Wprime = tables->get_from_scaled_root_powers((SmallNTTTables *)tables, m + i);

                uint64_t *X = operand + j1;
                uint64_t *Y = X + t;
                uint64_t currX;
                uint64_t Q;

                for (size_t j = j1; j < j2; j++) {
                    /* The Harvey butterfly: assume X, Y in [0, 2p), and return X', Y' in [0, 2p).
                     * X', Y' = X + WY, X - WY (mod p). */
                    currX = *X - (two_times_modulus & (uint64_t)(-1 * (int64_t)(*X >= two_times_modulus)));
                    multiply_uint64_hw64(Wprime, *Y, &Q);
                    Q = W * *Y - Q * modulus;
                    *X++ = currX + Q;
                    *Y++ = currX + (two_times_modulus - Q);
                }
            }
        }
        t >>= 1;
    }
}

/* Inverse negacyclic NTT using Harvey's butterfly. (See Patrick Longa and Michael Naehrig). */
void inverse_ntt_negacyclic_harvey_lazy(uint64_t *operand, const SmallNTTTables *tables)
{
    uint64_t modulus = tables->modulus->value;
    uint64_t two_times_modulus = modulus * 2;

    size_t n = (size_t)(1 << tables->coeff_count_power);
    size_t t = 1;

    for (size_t m = n; m > 1; m >>= 1) {
        size_t j1 = 0;
        size_t h = m >> 1;
        if (t >= 4) {
            for (size_t i = 0; i < h; i++) {
                size_t j2 = j1 + t;
                //log_warning("+++ h=%zu i=%zu\n", h, i);
                const uint64_t W = tables->get_from_inv_root_powers_div_two((SmallNTTTables *)tables, h + i);
                const uint64_t Wprime = tables->get_from_scaled_inv_root_powers_div_two((SmallNTTTables *)tables, h + i);
                //log_warning("@@ W=%lu Wprime=%lu\n", W, Wprime);
                uint64_t *U = operand + j1;
                uint64_t *V = U + t;
                uint64_t currU;
                uint64_t T;
                uint64_t H;

                for (size_t j = j1; j < j2; j += 4) {
                    T = two_times_modulus - *V + *U;
                    currU = *U + *V - (two_times_modulus & (uint64_t)(-1 * (int64_t)((*U << 1) >= T)));
                    *U++ = (currU + (modulus & (uint64_t)(-1 * (int64_t)(T & 1)))) >> 1;
                    multiply_uint64_hw64(Wprime, T, &H);
                    *V++ = T * W - H * modulus;

                    T = two_times_modulus - *V + *U;
                    currU = *U + *V - (two_times_modulus & (uint64_t)(-1 * (int64_t)((*U << 1) >= T)));
                    *U++ = (currU + (modulus & (uint64_t)(-1 * (int64_t)(T & 1)))) >> 1;
                    multiply_uint64_hw64(Wprime, T, &H);
                    *V++ = T * W - H * modulus;

                    T = two_times_modulus - *V + *U;
                    currU = *U + *V - (two_times_modulus & (uint64_t)(-1 * (int64_t)((*U << 1) >= T)));
                    *U++ = (currU + (modulus & (uint64_t)(-1 * (int64_t)(T & 1)))) >> 1;
                    multiply_uint64_hw64(Wprime, T, &H);
                    *V++ = T * W - H * modulus;

                    T = two_times_modulus - *V + *U;
                    currU = *U + *V - (two_times_modulus & (uint64_t)(-1 * (int64_t)((*U << 1) >= T)));
                    *U++ = (currU + (modulus & (uint64_t)(-1 * (int64_t)(T & 1)))) >> 1;
                    multiply_uint64_hw64(Wprime, T, &H);
                    *V++ = T * W - H * modulus;
                }
                j1 += (t << 1);
            }
        } else {
            for (size_t i = 0; i < h; i++) {
                size_t j2 = j1 + t;

                const uint64_t W = tables->get_from_inv_root_powers_div_two((SmallNTTTables *)tables, h + i);
                const uint64_t Wprime = tables->get_from_scaled_inv_root_powers_div_two((SmallNTTTables *)tables, h + i);

                uint64_t *U = operand + j1;
                uint64_t *V = U + t;
                uint64_t currU;
                uint64_t T;
                uint64_t masked_modulus;
                uint64_t carry;
                uint64_t H;

                for (size_t j = j1; j < j2; j++) {
                    /* Compute U - V + 2q */
                    T = two_times_modulus - *V + *U;

                    /* Cleverly check whether currU + currV >= two_times_modulus */
                    currU = *U + *V - (two_times_modulus & (uint64_t)(-1 * (int64_t)((*U << 1) >= T)));

                    /* Need to make it so that div2_uint_mod takes values that are > q.
                     * div2_uint_mod(U, modulusptr, coeff_uint64_count, U);
                     * We use also the fact that parity of currU is same as parity of T.
                     * Since our modulus is always so small that currU + masked_modulus < 2^64,
                     * we never need to worry about wrapping around when adding masked_modulus. */
                    //masked_modulus = modulus & (uint64_t)(-1 * (int64_t)(T & 1));
                    //carry = add_uint64_w_carry(currU, masked_modulus, 0, &currU);
                    //currU += modulus & (uint64_t)(-1 * (int64_t)(T & 1));
                    *U++ = (currU + (modulus & (uint64_t)(-1 * (int64_t)(T & 1)))) >> 1;

                    multiply_uint64_hw64(Wprime, T, &H);
                    /* effectively, the next two multiply perform multiply modulo beta = 2**wordsize. */
                    *V++ = W * T - H * modulus;
                }
                j1 += (t << 1);
            }
        }
        t <<= 1;
    }
}

void ntt_negacyclic_harvey(uint64_t *operand, const SmallNTTTables *tables)
{
    uint64_t modulus = tables->modulus->value;
    uint64_t two_times_modulus = modulus * 2;
    /* Finally maybe we need to reduce every coefficient modulo q, but we know that
     * they are in the range [0, 4q). Since word size is controlled this is fast. */
    size_t n = (size_t)(1 << tables->coeff_count_power);

    ntt_negacyclic_harvey_lazy(operand, tables);

    for (; n--; operand++) {
        if (*operand >= two_times_modulus) {
            *operand -= two_times_modulus;
        }
        if (*operand >= modulus) {
            *operand -= modulus;
        }
    }
}

void inverse_ntt_negacyclic_harvey(uint64_t *operand, const SmallNTTTables *tables)
{
    uint64_t modulus = tables->modulus->value;
    size_t n = (size_t)(1 << tables->coeff_count_power);

    inverse_ntt_negacyclic_harvey_lazy(operand, tables);

    /* Final adjustments; compute a[j] = a[j] * n^{-1} mod q.
     * We incorporated the final adjustment in the butterfly.
     * Only need to reduce here. */
    for (; n--; operand++) {
        if (*operand >= modulus)
            *operand -= modulus;
    }
}

static void
ntt_powers_of_primitive_root(SmallNTTTables *self, uint64_t root, uint64_t *dest)
{
    uint64_t *dest_start = dest;
    
    *dest_start = 1;
    for (size_t i = 1; i < self->coeff_count; i++) {
        uint64_t *next_dest = dest_start + reverse_bits(i, self->coeff_count_power);
        *next_dest = multiply_uint_uint_smallmod(*dest, root, self->modulus);
        dest = next_dest;
    }
}

static void
ntt_scale_powers_of_primitive_root(SmallNTTTables *self, const uint64_t *input, uint64_t *dest)
{
    for (size_t i = 0; i < self->coeff_count; i++, input++, dest++) {
        uint64_t wide_quotient[2] = { 0, 0 };
        uint64_t wide_coeff[2] = { 0, *input };

        divide_uint128_uint64_inplace(wide_coeff, self->modulus->value, wide_quotient);
        *dest = wide_quotient[0];
    }
}

static int init_smallntt_tables(SmallNTTTables *self)
{
    log_trace("init_smallntt_tables(%p)\n", self);
    self->generated = false;
    self->root = 0;
    self->root_powers = NULL;
    self->coeff_count_power = 0;
    self->coeff_count = 0;
    self->inv_degree_modulo = 0;
    self->modulus = NULL;
    self->scaled_root_powers = NULL;
    self->inv_root_powers_div_two = NULL;
    self->scaled_inv_root_powers_div_two = NULL;
    self->inv_root_powers = NULL;
    self->scaled_inv_root_powers = NULL;

    if (!self->generate(self, self->coeff_count_power, NULL)) {
        log_fatal("init_smallntt_tables(%p) failed!n", self);
        return -1;
    } else
        return 0;
}

static void reset_smallntt_tables(SmallNTTTables *self)
{
    log_trace("reset_smallntt_tables(%p)\n", self);
    self->generated = false;
    self->root = 0;
    self->inv_degree_modulo = 0;
    self->coeff_count_power = 0;
    self->coeff_count = 0;
#if 0
    if (self->modulus) {
        del_Zmodulus(self->modulus);
        self->modulus = NULL;
    }
#endif
    if (self->root_powers) {
        hedge_free(self->root_powers);
        self->root_powers = NULL;
    }
    if (self->scaled_root_powers) {
        hedge_free(self->scaled_root_powers);
        self->scaled_root_powers = NULL;
    }
    if (self->inv_root_powers) {
        hedge_free(self->inv_root_powers);
        self->inv_root_powers = NULL;
    }
    if (self->scaled_inv_root_powers) {
        hedge_free(self->scaled_inv_root_powers);
        self->scaled_inv_root_powers = NULL;
    }
    if (self->inv_root_powers_div_two) {
        hedge_free(self->inv_root_powers_div_two);
        self->inv_root_powers_div_two = NULL;
    }
    if (self->scaled_inv_root_powers_div_two) {
        hedge_free(self->scaled_inv_root_powers_div_two);
        self->scaled_inv_root_powers_div_two = NULL;
    }
}

static bool generate_smallntt_tables(SmallNTTTables *self,
                                     int coeff_count_power,
                                     const Zmodulus *modulus)
{
    uint64_t inverse_root;
    uint64_t degree_unit;

    if (coeff_count_power == 0 || !modulus) {
        /* let's ignore trivial requests.
         * In many cases, here is the consequence of smallNTTTabls instance creation
         * sequence, meaningless calling generate method with empty arguments.
         */
        return true;
    }
    self->reset(self);

    assert((coeff_count_power >= get_power_of_two(HEDGE_POLY_MOD_DEGREE_MIN)) &&
           (coeff_count_power <= get_power_of_two(HEDGE_POLY_MOD_DEGREE_MAX)));

    self->coeff_count_power = coeff_count_power;
    self->coeff_count = (size_t)(1 << coeff_count_power);

    self->root_powers = hedge_zalloc(sizeof(uint64_t) * self->coeff_count);
    self->inv_root_powers = hedge_zalloc(sizeof(uint64_t) * self->coeff_count);
    self->scaled_root_powers = hedge_zalloc(sizeof(uint64_t) * self->coeff_count);
    self->scaled_inv_root_powers = hedge_zalloc(sizeof(uint64_t) * self->coeff_count);
    self->inv_root_powers_div_two = hedge_zalloc(sizeof(uint64_t) * self->coeff_count);
    self->scaled_inv_root_powers_div_two = hedge_zalloc(sizeof(uint64_t) * self->coeff_count);
    self->modulus = (Zmodulus *)modulus;

    if (!try_minimal_primitive_root(self->coeff_count << 1, modulus, &self->root)) {
        self->reset(self);
        return false;
    }

    if (!try_invert_uint_smallmod(self->root, modulus, &inverse_root)) {
        self->reset(self);
        return false;
    }

    self->ntt_powers_of_primitive_root(self, self->root, self->root_powers);
    self->ntt_scale_powers_of_primitive_root(self,
            self->root_powers, self->scaled_root_powers);

    self->ntt_powers_of_primitive_root(self, inverse_root, self->inv_root_powers);
    self->ntt_scale_powers_of_primitive_root(self,
            self->inv_root_powers, self->scaled_inv_root_powers);

    for (size_t i = 0; i < self->coeff_count; i++) {
        self->inv_root_powers_div_two[i] =
            div2_uint_smallmod(self->inv_root_powers[i], modulus);
        //if (i >= 8100)
        //log_warning("self->inv_root_powers_div_two[%zu]=%lu\n", i, self->inv_root_powers_div_two[i]);
        
        
    }

    self->ntt_scale_powers_of_primitive_root(self,
            self->inv_root_powers_div_two, self->scaled_inv_root_powers_div_two);

    uint64_t degree_uint = (uint64_t)self->coeff_count;
    self->generated =
        try_invert_uint_smallmod(degree_uint, self->modulus, &self->inv_degree_modulo);

    if (!self->generated) {
        self->reset(self);
        return false;
    }

    return true;
}

static uint64_t get_from_root_powers(SmallNTTTables *self, size_t index)
{
    return self->root_powers[index];
}

static uint64_t get_from_scaled_root_powers(SmallNTTTables *self, size_t index)
{
    return self->scaled_root_powers[index];
}

static uint64_t get_from_inv_root_powers(SmallNTTTables *self, size_t index)
{
    return self->inv_root_powers[index];
}

static uint64_t get_from_scaled_inv_root_powers(SmallNTTTables *self, size_t index)
{
    return self->scaled_inv_root_powers[index];
}

static uint64_t get_from_inv_root_powers_div_two(SmallNTTTables *self, size_t index)
{
    return self->inv_root_powers_div_two[index];
}

static uint64_t get_from_scaled_inv_root_powers_div_two(SmallNTTTables *self, size_t index)
{
    return self->scaled_inv_root_powers_div_two[index];
}

static uint64_t get_inv_degree_modulo(SmallNTTTables *self)
{
    return self->inv_degree_modulo;
}

static void free_smallntt_tables(SmallNTTTables *self)
{
    hedge_free(self->root_powers);
    hedge_free(self->scaled_root_powers);
    hedge_free(self->inv_root_powers);
    hedge_free(self->scaled_inv_root_powers);
    hedge_free(self->inv_root_powers_div_two);
    hedge_free(self->scaled_inv_root_powers_div_two);
    //del_Zmodulus(self->modulus);
}

SmallNTTTables *new_SmallNTTTables(void)
{
    SmallNTTTables *obj = hedge_zalloc(sizeof(SmallNTTTables));

    if (!obj)
        return NULL;

    *obj = (SmallNTTTables) {
        .init = init_smallntt_tables,
        .free = free_smallntt_tables,
        .generate = generate_smallntt_tables,
        .reset = reset_smallntt_tables,
        .get_from_root_powers = get_from_root_powers,
        .get_from_scaled_root_powers = get_from_scaled_root_powers,
        .get_from_inv_root_powers = get_from_inv_root_powers,
        .get_from_scaled_inv_root_powers = get_from_scaled_inv_root_powers,
        .get_from_inv_root_powers_div_two = get_from_inv_root_powers_div_two,
        .get_from_scaled_inv_root_powers_div_two = get_from_scaled_inv_root_powers_div_two,
        .ntt_powers_of_primitive_root = ntt_powers_of_primitive_root,
        .ntt_scale_powers_of_primitive_root = ntt_scale_powers_of_primitive_root,
    };

    if (obj->init(obj) < 0) {
        obj->free(obj);
        hedge_free(obj);
        return NULL;
    } else
        return obj;
}

void del_SmallNTTTables(SmallNTTTables *self)
{
    self->free(self);
    hedge_free(self);
}