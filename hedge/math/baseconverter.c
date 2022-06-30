#ifdef __cplusplus
extern "C" {
#endif

#include "baseconverter.h"
#include <assert.h>

#include "math/uintcore.h"
#include "math/polycore.h"
#include "math/uintarith.h"
#include "math/uintarithsmallmod.h"
#include "math/uintarithmod.h"
#include "math/polyarithsmallmod.h"
#include "math/smallntt.h"
#include "math/smallmodulus.h"

const uint64_t aux_small_mods[] = {
    0x1fffffffffb40001, 0x1fffffffff500001, 0x1fffffffff380001, 0x1fffffffff000001,
    0x1ffffffffef00001, 0x1ffffffffee80001, 0x1ffffffffeb40001, 0x1ffffffffe780001,
    0x1ffffffffe600001, 0x1ffffffffe4c0001, 0x1ffffffffdf40001, 0x1ffffffffdac0001,
    0x1ffffffffda40001, 0x1ffffffffc680001, 0x1ffffffffc000001, 0x1ffffffffb880001,
    0x1ffffffffb7c0001, 0x1ffffffffb300001, 0x1ffffffffb1c0001, 0x1ffffffffadc0001,
    0x1ffffffffa400001, 0x1ffffffffa140001, 0x1ffffffff9d80001, 0x1ffffffff9140001,
    0x1ffffffff8ac0001, 0x1ffffffff8a80001, 0x1ffffffff81c0001, 0x1ffffffff7800001,
    0x1ffffffff7680001, 0x1ffffffff7080001, 0x1ffffffff6c80001, 0x1ffffffff6140001,
    0x1ffffffff5f40001, 0x1ffffffff5700001, 0x1ffffffff4bc0001, 0x1ffffffff4380001,
    0x1ffffffff3240001, 0x1ffffffff2dc0001, 0x1ffffffff1a40001, 0x1ffffffff11c0001,
    0x1ffffffff0fc0001, 0x1ffffffff0d80001, 0x1ffffffff0c80001, 0x1ffffffff08c0001,
    0x1fffffffefd00001, 0x1fffffffef9c0001, 0x1fffffffef600001, 0x1fffffffeef40001,
    0x1fffffffeed40001, 0x1fffffffeed00001, 0x1fffffffeebc0001, 0x1fffffffed540001,
    0x1fffffffed440001, 0x1fffffffed2c0001, 0x1fffffffed200001, 0x1fffffffec940001,
    0x1fffffffec6c0001, 0x1fffffffebe80001, 0x1fffffffebac0001, 0x1fffffffeba40001,
    0x1fffffffeb4c0001, 0x1fffffffeb280001, 0x1fffffffea780001, 0x1fffffffea440001,
    0x1fffffffe9f40001, 0x1fffffffe97c0001, 0x1fffffffe9300001, 0x1fffffffe8d00001,
    0x1fffffffe8400001, 0x1fffffffe7cc0001, 0x1fffffffe7bc0001, 0x1fffffffe7a80001,
    0x1fffffffe7600001, 0x1fffffffe7500001, 0x1fffffffe6fc0001, 0x1fffffffe6d80001,
    0x1fffffffe6ac0001, 0x1fffffffe6000001, 0x1fffffffe5d40001, 0x1fffffffe5a00001,
    0x1fffffffe5940001, 0x1fffffffe54c0001, 0x1fffffffe5340001, 0x1fffffffe4bc0001,
    0x1fffffffe4a40001, 0x1fffffffe3fc0001, 0x1fffffffe3540001, 0x1fffffffe2b00001,
    0x1fffffffe2680001, 0x1fffffffe0480001, 0x1fffffffe00c0001, 0x1fffffffdfd00001,
    0x1fffffffdfc40001, 0x1fffffffdf700001, 0x1fffffffdf340001, 0x1fffffffdef80001,
    0x1fffffffdea80001, 0x1fffffffde680001, 0x1fffffffde000001, 0x1fffffffdde40001,
    0x1fffffffddd80001, 0x1fffffffddd00001, 0x1fffffffddb40001, 0x1fffffffdd780001,
    0x1fffffffdd4c0001, 0x1fffffffdcb80001, 0x1fffffffdca40001, 0x1fffffffdc380001,
    0x1fffffffdc040001, 0x1fffffffdbb40001, 0x1fffffffdba80001, 0x1fffffffdb9c0001,
    0x1fffffffdb740001, 0x1fffffffdb380001, 0x1fffffffda600001, 0x1fffffffda340001,
    0x1fffffffda180001, 0x1fffffffd9700001, 0x1fffffffd9680001, 0x1fffffffd9440001,
    0x1fffffffd9080001, 0x1fffffffd8c80001, 0x1fffffffd8800001, 0x1fffffffd82c0001,
    0x1fffffffd7cc0001, 0x1fffffffd7b80001, 0x1fffffffd7840001, 0x1fffffffd73c0001
};

static void init(struct base_converter *self)
{
    self->generate(self, NULL, 0, NULL);
}

static void init_with_parms(
        struct base_converter *self,
        vector(Zmodulus) *coeff_base,
        size_t coeff_count,
        const Zmodulus *small_plain_mod)
{
    self->generate(self, coeff_base, coeff_count, small_plain_mod);
}

static void generate_baseconverter(
        struct base_converter *self,
        vector(Zmodulus) *coeff_base,
        size_t coeff_count,
        const Zmodulus *small_plain_mod)
{
    int total_coeff_bit_count;
    int coeff_count_power = get_power_of_two(coeff_count);
    size_t i, j;
    void *tmp_coeff;
    void *tmp_aux;
    uint64_t *aux_products_array;
    void *coeff_products_all;
    void *tmp_products_all;
    void *aux_products_all;
    void *tmp_aux_products_all;

    if (!coeff_base || !small_plain_mod || !coeff_count) {
        /* let's ignore trivial requests.
         * In many cases, here is the consequence of smallNTTTabls instance creation
         * sequence, meaningless calling generate method with empty arguments.
         */
        return;
    }

#ifdef HEDGE_DEBUG
    assert(get_power_of_two(coeff_count) >= 0);
    assert(coeff_base->size >= HEDGE_COEFF_MOD_COUNT_MIN &&
           coeff_base->size <= HEDGE_COEFF_MOD_COUNT_MAX);
#endif

    /**
     * Perform all the required pre-computations and populate the tables
     */
    self->reset(self);

    self->m_sk = new_Zmodulus(0x1fffffffffe00001);
    self->m_tilde = new_Zmodulus((uint64_t)1 << 32);
    self->gamma = new_Zmodulus(0x1fffffffffc80001);
    self->small_plain_mod = new_Zmodulus(small_plain_mod->value);
    self->coeff_count = coeff_count;
    self->coeff_base_mod_count = coeff_base->size;
    self->aux_base_mod_count = coeff_base->size;
    // TODO: check here for coeff_count_power

    /* In some cases we might need to increase the size of the aux base by one, namely
     * we require K * n * t * q^2 < q * prod_i m_i * m_sk, where K takes into account
     * cross terms when larger size ciphertexts are used, and n is the "delta factor"
     * for the ring. We reserve 32 bits for K * n. Here the coeff modulus primes q_i
     * are bounded to be 60 bits, and all m_i, m_sk are 61 bits. */
    total_coeff_bit_count = 0;
    int idx;
    Zmodulus* mod;

    iterate_vector(idx, mod, coeff_base) {
        total_coeff_bit_count += mod->value;
    }

    if (32 + self->small_plain_mod->bit_count + total_coeff_bit_count
        >= 61 * (int)self->coeff_base_mod_count + 61) {
        self->aux_base_mod_count++;
    }

    self->bsk_base_mod_count = self->aux_base_mod_count + 1;
    self->plain_gamma_count = 2;
    self->plain_gamma_array = hedge_zalloc(self->plain_gamma_count * sizeof(self->plain_gamma_count));
    /* Size check; should always pass */
    assert(product_fits_in(self->coeff_count, self->coeff_base_mod_count));
    assert(product_fits_in(self->coeff_count, self->aux_base_mod_count));
    assert(product_fits_in(self->coeff_count, self->bsk_base_mod_count));

    self->coeff_products_mod_plain_gamma_array
            = hedge_malloc(sizeof(uint64_t) * self->plain_gamma_count);
    for (i = 0; i < self->plain_gamma_count; i++) {
        self->coeff_products_mod_plain_gamma_array[i]
                = hedge_malloc(sizeof(uint64_t) * self->coeff_base_mod_count);        
    }

    /* moduli reference arrays derivated from Hedge encyption parameters */
    self->coeff_base_array
            = hedge_malloc(sizeof(Zmodulus *) * coeff_base->size);
    for (i = 0; i < coeff_base->size; i++) {
        self->coeff_base_array[i] = coeff_base->at(coeff_base, i);
    }

    self->aux_base_array
            = hedge_malloc(sizeof(Zmodulus *) * self->aux_base_mod_count);
    self->bsk_base_array
            = hedge_malloc(sizeof(Zmodulus *) * self->bsk_base_mod_count);
    for (i = 0; i < self->aux_base_mod_count; i++) {
        self->aux_base_array[i] = new_Zmodulus(aux_small_mods[i]);
        self->bsk_base_array[i] = new_Zmodulus(aux_small_mods[i]);
    }
    self->bsk_base_array[self->bsk_base_mod_count - 1] = self->m_sk;

    /* Generate Bsk U {mtilde} small ntt tables which is used in Evaluator */
    self->bsk_small_ntt_tables
            = hedge_malloc(sizeof(SmallNTTTables *) * self->bsk_base_mod_count);    
    for (i = 0; i < self->bsk_base_mod_count; i++) {
        self->bsk_small_ntt_tables[i] = new_SmallNTTTables();
        if (!self->bsk_small_ntt_tables[i]->generate(self->bsk_small_ntt_tables[i],
                                                     coeff_count_power,
                                                     self->bsk_base_array[i])) {
            self->reset(self);
            return;
        }
    }

    /* Generate punctured products of coeff moduli */
    self->coeff_products_array = allocate_zero_uint(
                self->coeff_base_mod_count * self->coeff_base_mod_count);
    tmp_coeff = allocate_uint(self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        self->coeff_products_array[i * self->coeff_base_mod_count] = 1;
        for (j = 0; j < self->coeff_base_mod_count; j++) {
            if (i != j) {
                multiply_uint_uint64(self->coeff_products_array + (i * self->coeff_base_mod_count),
                        self->coeff_base_mod_count,
                        self->coeff_base_array[j]->value, self->coeff_base_mod_count,
                        (uint64_t *)tmp_coeff);
                set_uint_uint((uint64_t *)tmp_coeff,
                        self->coeff_base_mod_count,
                        self->coeff_products_array + (i * self->coeff_base_mod_count));
            }
        }
    }

    /* Generate punctured products of aux moduli */
    aux_products_array = allocate_zero_uint(
                self->aux_base_mod_count * self->aux_base_mod_count);
    tmp_aux = allocate_uint(self->aux_base_mod_count);
    for (i = 0; i < self->aux_base_mod_count; i++) {
        aux_products_array[i * self->aux_base_mod_count] = 1;
        for (j = 0; j < self->aux_base_mod_count; j++) {
            if (i != j) {
                multiply_uint_uint64(aux_products_array + (i * self->aux_base_mod_count),
                        self->aux_base_mod_count,
                        self->aux_base_array[j]->value, self->aux_base_mod_count,
                        tmp_aux);
                set_uint_uint(tmp_aux,
                        self->aux_base_mod_count,
                        aux_products_array + (i * self->aux_base_mod_count));
            }
        }
    }

    /* Compute auxiliary base products mod m_sk */
    self->aux_base_products_mod_msk_array = allocate_uint(self->aux_base_mod_count);
    for (i = 0; i < self->aux_base_mod_count; i++) {
        self->aux_base_products_mod_msk_array[i]
                = modulo_uint(aux_products_array + (i * self->aux_base_mod_count),
                        self->aux_base_mod_count, self->m_sk);
    }

    /* Compute inverse coeff base mod coeff base array (qi^(-1)) mod qi and
     * mtilde inv coeff products mod auxiliary moduli  (m_tilda*qi^(-1)) mod qi */
    self->inv_coeff_base_products_mod_coeff_array
            = allocate_uint(self->coeff_base_mod_count);
    self->mtilde_inv_coeff_base_products_mod_coeff_array
            = allocate_uint(self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        self->inv_coeff_base_products_mod_coeff_array[i]
                = modulo_uint(self->coeff_products_array + (i * self->coeff_base_mod_count),
                        self->coeff_base_mod_count,
                        self->coeff_base_array[i]);
        if (!try_invert_uint_smallmod(self->inv_coeff_base_products_mod_coeff_array[i],
            self->coeff_base_array[i],
            &self->inv_coeff_base_products_mod_coeff_array[i])) {
            self->reset(self);
            return;
        }
        self->mtilde_inv_coeff_base_products_mod_coeff_array[i]
                = multiply_uint_uint_smallmod(self->inv_coeff_base_products_mod_coeff_array[i],
                        self->m_tilde->value, self->coeff_base_array[i]);
    }

    /* Compute inverse auxiliary moduli mod auxiliary moduli (mi^(-1)) mod mi */
    self->inv_aux_base_products_mod_aux_array = allocate_uint(self->aux_base_mod_count);
    for (i = 0; i < self->aux_base_mod_count; i++) {
        self->inv_aux_base_products_mod_aux_array[i]
                = modulo_uint(aux_products_array + (i * self->aux_base_mod_count),
                        self->aux_base_mod_count,
                        self->aux_base_array[i]);
        if (!try_invert_uint_smallmod(self->inv_aux_base_products_mod_aux_array[i],
            self->aux_base_array[i],
            &self->inv_aux_base_products_mod_aux_array[i])) {
            self->reset(self);
            return;
        }
    }

    /* Compute coeff modulus products mod mtilde (qi) mod m_tilde_ */
    self->coeff_base_products_mod_mtilde_array = allocate_uint(self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        self->coeff_base_products_mod_mtilde_array[i]
                = modulo_uint(self->coeff_products_array + (i * self->coeff_base_mod_count),
                        self->coeff_base_mod_count,
                        self->m_tilde);
    }

    /* Compute coeff modulus products mod auxiliary moduli (qi) mod mj U {msk} */
    self->coeff_base_products_mod_aux_bsk_array
            = (uint64_t **)allocate_uint(self->bsk_base_mod_count);
    for (i = 0; i < self->aux_base_mod_count; i++) {
        self->coeff_base_products_mod_aux_bsk_array[i]
                = allocate_uint(self->coeff_base_mod_count);
        for (j = 0; j < self->coeff_base_mod_count; j++) {
            self->coeff_base_products_mod_aux_bsk_array[i][j]
                    = modulo_uint(self->coeff_products_array + (j * self->coeff_base_mod_count),
                            self->coeff_base_mod_count,
                            self->aux_base_array[i]);
        }
    }

    /* Add qi mod msk at the end of the array */
    self->coeff_base_products_mod_aux_bsk_array[self->bsk_base_mod_count - 1]
            = allocate_uint(self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        self->coeff_base_products_mod_aux_bsk_array[self->bsk_base_mod_count - 1][i]
                = modulo_uint(self->coeff_products_array + (i * self->coeff_base_mod_count),
                        self->coeff_base_mod_count,
                        self->m_sk);
    }

    /* Compute auxiliary moduli products mod coeff moduli (mj) mod qi */
    self->aux_base_products_mod_coeff_array
            = (uint64_t **)allocate_uint(self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        self->aux_base_products_mod_coeff_array[i]
                = allocate_uint(self->aux_base_mod_count);
        for (j = 0; j < self->aux_base_mod_count; j++) {
            self->aux_base_products_mod_coeff_array[i][j]
                    = modulo_uint(aux_products_array + (j * self->aux_base_mod_count),
                            self->aux_base_mod_count, self->coeff_base_array[i]);
        }
    }

    /* Compute coeff moduli products inverse mod auxiliary mods  (qi^(-1)) mod mj U {msk} */
    coeff_products_all = allocate_uint(self->coeff_base_mod_count);
    tmp_products_all = allocate_uint(self->coeff_base_mod_count);
    set_uint(1, self->coeff_base_mod_count, (uint64_t *)coeff_products_all);

    /* Compute the product of all coeff moduli */
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        multiply_uint_uint64((uint64_t *)coeff_products_all,
                    self->coeff_base_mod_count,
                    self->coeff_base_array[i]->value,
                    self->coeff_base_mod_count,
                    (uint64_t *)tmp_products_all);
        set_uint_uint((uint64_t *)tmp_products_all,
                    self->coeff_base_mod_count,
                    (uint64_t *)coeff_products_all);
    }

    /* Compute inverses of coeff_products_all modulo aux moduli */
    self->inv_coeff_products_all_mod_aux_bsk_array = allocate_uint(self->bsk_base_mod_count);
    for (i = 0; i < self->aux_base_mod_count; i++) {
        self->inv_coeff_products_all_mod_aux_bsk_array[i]
                = modulo_uint((uint64_t *)coeff_products_all,
                        self->coeff_base_mod_count, self->aux_base_array[i]);
        if (!try_invert_uint_smallmod(self->inv_coeff_products_all_mod_aux_bsk_array[i],
            self->aux_base_array[i],
            &self->inv_coeff_products_all_mod_aux_bsk_array[i])) {
            self->reset(self);
            return;
        }
    }

    /* Add product of all coeffs mod msk at the end of the array */
    self->inv_coeff_products_all_mod_aux_bsk_array[self->bsk_base_mod_count - 1]
            = modulo_uint((uint64_t *)coeff_products_all,
                    self->coeff_base_mod_count, self->m_sk);
    if (!try_invert_uint_smallmod(self->inv_coeff_products_all_mod_aux_bsk_array[self->bsk_base_mod_count - 1],
        self->m_sk,
        &self->inv_coeff_products_all_mod_aux_bsk_array[self->bsk_base_mod_count - 1])) {
        self->reset(self);
        return;
    }

    /* Compute the products of all aux moduli */
    aux_products_all = allocate_uint(self->aux_base_mod_count);
    tmp_aux_products_all = allocate_uint(self->aux_base_mod_count);
    set_uint(1, self->aux_base_mod_count, (uint64_t *)aux_products_all);
    for (i = 0; i < self->aux_base_mod_count; i++) {
        multiply_uint_uint64((uint64_t *)aux_products_all,
                self->aux_base_mod_count,
                self->aux_base_array[i]->value,
                self->aux_base_mod_count,
                (uint64_t *)tmp_aux_products_all);
        set_uint_uint((uint64_t *)tmp_aux_products_all,
                self->aux_base_mod_count,
                (uint64_t *)aux_products_all);
    }

    /* Compute the auxiliary products inverse mod m_sk_ (M-1) mod m_sk_ */
    self->inv_aux_products_mod_msk
            = modulo_uint((uint64_t *)aux_products_all, self->aux_base_mod_count, self->m_sk);
    if (!try_invert_uint_smallmod(self->inv_aux_products_mod_msk,
        self->m_sk, &self->inv_aux_products_mod_msk)) {
        self->reset(self);
        return;
    }

    /* Compute auxiliary products all mod coefficient moduli */
    self->aux_products_all_mod_coeff_array = allocate_uint(self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        self->aux_products_all_mod_coeff_array[i]
                = modulo_uint((uint64_t *)aux_products_all,
                        self->aux_base_mod_count, self->coeff_base_array[i]);
    }

    /* Compute m_tilde inverse mod bsk base */
    self->inv_mtilde_mod_bsk_array = allocate_uint(self->bsk_base_mod_count);
    for (i = 0; i < self->aux_base_mod_count; i++) {
        if (!try_invert_uint_smallmod(self->m_tilde->value % self->aux_base_array[i]->value,
            self->aux_base_array[i], &self->inv_mtilde_mod_bsk_array[i])) {
            self->reset(self);
            return;
        }
    }

    /* Add m_tilde inverse mod msk at the end of the array */
    if (!try_invert_uint_smallmod(self->m_tilde->value % self->m_sk->value, self->m_sk,
        &self->inv_mtilde_mod_bsk_array[self->bsk_base_mod_count - 1])) {
        self->reset(self);
        return;
    }

    /* Compute coeff moduli products inverse mod m_tilde */
    self->inv_coeff_products_mod_mtilde
            = modulo_uint((uint64_t *)coeff_products_all, self->coeff_base_mod_count, self->m_tilde);
    if (!try_invert_uint_smallmod(self->inv_coeff_products_mod_mtilde,
        self->m_tilde, &self->inv_coeff_products_mod_mtilde)) {
        self->reset(self);
        return;
    }

    /* Compute coeff base products all mod Bsk */
    self->coeff_products_all_mod_bsk_array = allocate_uint(self->bsk_base_mod_count);
    for (i = 0; i < self->aux_base_mod_count; i++) {
        self->coeff_products_all_mod_bsk_array[i]
                = modulo_uint((uint64_t *)coeff_products_all,
                        self->coeff_base_mod_count, self->aux_base_array[i]);
    }

    /* Add coeff base products all mod m_sk_ at the end of the array */
    self->coeff_products_all_mod_bsk_array[self->bsk_base_mod_count - 1]
            = modulo_uint((uint64_t *)coeff_products_all, self->coeff_base_mod_count, self->m_sk);

    /* Compute inverses of last coeff base modulus modulo the first ones for
     * modulus switching/rescaling. */
    self->inv_last_coeff_mod_array = allocate_uint(self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count - 1; i++) {
        if (!try_mod_inverse(self->coeff_base_array[self->coeff_base_mod_count - 1]->value,
            self->coeff_base_array[i]->value, &self->inv_last_coeff_mod_array[i])) {
            self->reset(self);
            return;
        }
    }

    /* Generate plain gamma array of small_plain_mod_ is set to non-zero.
     * Otherwise assume we use CKKS and no plain_modulus is needed. */
    if (self->small_plain_mod->value != 0) {
        self->plain_gamma_array[0] = self->small_plain_mod;
        self->plain_gamma_array[1] = self->gamma;

        for (i = 0; i < self->plain_gamma_count; i++) {
            self->coeff_products_mod_plain_gamma_array[i] = allocate_uint(self->coeff_base_mod_count);
            for (j = 0; j < self->coeff_base_mod_count; j++) {
                self->coeff_products_mod_plain_gamma_array[i][j]
                        = modulo_uint(
                                self->coeff_products_array + (j * self->coeff_base_mod_count),
                                self->coeff_base_mod_count, self->plain_gamma_array[i]
                        );
            }
        }

        /* Compute inverse of all coeff moduli products mod plain gamma */
        self->neg_inv_coeff_products_all_mod_plain_gamma_array
                    = allocate_uint(self->plain_gamma_count);
        for (i = 0; i < self->plain_gamma_count; i++) {
            uint64_t temp = modulo_uint((uint64_t *)coeff_products_all,
                                    self->coeff_base_mod_count, self->plain_gamma_array[i]);
            self->neg_inv_coeff_products_all_mod_plain_gamma_array[i]
                    = negate_uint_smallmod(temp, self->plain_gamma_array[i]);
            if (!try_invert_uint_smallmod(self->neg_inv_coeff_products_all_mod_plain_gamma_array[i],
                self->plain_gamma_array[i], &self->neg_inv_coeff_products_all_mod_plain_gamma_array[i])) {
                self->reset(self);
                return;
            }
        }

        /* Compute inverse of gamma mod plain modulus */
        self->inv_gamma_mod_plain
                = modulo_uint((const uint64_t *)self->gamma->value, self->gamma->uint64_count, self->small_plain_mod);
        if (!try_invert_uint_smallmod(self->inv_gamma_mod_plain,
            self->small_plain_mod, &self->inv_gamma_mod_plain)) {
            self->reset(self);
            return;
        }

        /* Compute plain_gamma product mod coeff base moduli */
        self->plain_gamma_product_mod_coeff_array = allocate_uint(self->coeff_base_mod_count);
        for (i = 0; i < self->coeff_base_mod_count; i++) {
            self->plain_gamma_product_mod_coeff_array[i]
                    = multiply_uint_uint_smallmod(self->small_plain_mod->value,
                            self->gamma->value, self->coeff_base_array[i]);
        }
    }

    hedge_free(tmp_coeff);
    hedge_free(tmp_aux);
    hedge_free(aux_products_array);
    hedge_free(coeff_products_all);
    hedge_free(tmp_products_all);
    hedge_free(aux_products_all);
    hedge_free(tmp_aux_products_all);

    self->generated = true;
}

static void reset(struct base_converter *self)
{
    size_t i;
    self->generated = false;

    if (self->coeff_base_array) {
        /** Removing reference array only;
         * Substantial coeff bases vector is freed
         * when encyption parameters instance is destroyed.*/
        hedge_free(self->coeff_base_array);
        self->coeff_base_array = NULL;
    }
    if (self->aux_base_array) {
        for (i = 0; i < self->aux_base_mod_count; i++) {
            if (self->aux_base_array[i]) {
                del_Zmodulus(self->aux_base_array[i]);
                self->aux_base_array[i] = NULL;
            }
        }
        hedge_free(self->aux_base_array);
        self->aux_base_array = NULL;
    }
    if (self->bsk_base_array) {
        for (i = 0; i < self->bsk_base_mod_count; i++) {
            if (self->bsk_base_array[i]) {
                del_Zmodulus(self->bsk_base_array[i]);
                self->bsk_base_array[i] = NULL;
            }
        }
        hedge_free(self->bsk_base_array);
        self->bsk_base_array = NULL;
    }

    if (self->plain_gamma_array) {
        /** Removing reference array only;
         * Substantial Zmodulus will be freed hereafter.*/
        hedge_free(self->plain_gamma_array);
        self->plain_gamma_array = NULL;
    }
    if (self->m_tilde) {
        del_Zmodulus(self->m_tilde);
        self->m_tilde = NULL;
    }
    /* self->m_sk is already freed. because 
        self->bsk_base_array[self->bsk_base_mod_count -1] = self->m_sk
        //if (self->m_sk) {
            //del_Zmodulus(self->m_sk);
            //self->m_sk = NULL;
        //}
    */
    if (self->small_plain_mod) {
        del_Zmodulus(self->small_plain_mod);
        self->small_plain_mod = NULL;
    }
    if (self->gamma) {
        del_Zmodulus(self->gamma);
        self->gamma = NULL;
    }
    if (self->coeff_products_array) {
        hedge_free(self->coeff_products_array);
        self->coeff_products_array = NULL;
    }
    if (self->mtilde_inv_coeff_base_products_mod_coeff_array) {
        hedge_free(self->mtilde_inv_coeff_base_products_mod_coeff_array);
        self->mtilde_inv_coeff_base_products_mod_coeff_array = NULL;
    }
    if (self->inv_aux_base_products_mod_aux_array) {
        hedge_free(self->inv_aux_base_products_mod_aux_array);
        self->inv_aux_base_products_mod_aux_array = NULL;
    }
    if (self->inv_coeff_products_all_mod_aux_bsk_array) {
        hedge_free(self->inv_coeff_products_all_mod_aux_bsk_array);
        self->inv_coeff_products_all_mod_aux_bsk_array = NULL;
    }
    if (self->inv_coeff_base_products_mod_coeff_array) {
        hedge_free(self->inv_coeff_base_products_mod_coeff_array);
        self->inv_coeff_base_products_mod_coeff_array = NULL;
    }
    if (self->aux_base_products_mod_coeff_array) {
        for (i = 0; i < self->coeff_base_mod_count; i++) {
            hedge_free(self->aux_base_products_mod_coeff_array[i]);
        }
        hedge_free(self->aux_base_products_mod_coeff_array);
        self->aux_base_products_mod_coeff_array = NULL;
    }
    if (self->coeff_base_products_mod_aux_bsk_array) {
        for (i = 0; i < self->aux_base_mod_count; i++) {
            hedge_free(self->coeff_base_products_mod_aux_bsk_array[i]);
        }
        hedge_free(self->coeff_base_products_mod_aux_bsk_array[self->bsk_base_mod_count - 1]);
        hedge_free(self->coeff_base_products_mod_aux_bsk_array);
        self->coeff_base_products_mod_aux_bsk_array = NULL;
    }
    if (self->coeff_base_products_mod_mtilde_array) {
        hedge_free(self->coeff_base_products_mod_mtilde_array);
        self->coeff_base_products_mod_mtilde_array = NULL;
    }
    if (self->aux_base_products_mod_msk_array) {
        hedge_free(self->aux_base_products_mod_msk_array);
        self->aux_base_products_mod_msk_array = NULL;
    }
    if (self->aux_products_all_mod_coeff_array) {
        hedge_free(self->aux_products_all_mod_coeff_array);
        self->aux_products_all_mod_coeff_array = NULL;
    }
    if (self->inv_mtilde_mod_bsk_array) {
        hedge_free(self->inv_mtilde_mod_bsk_array);
        self->inv_mtilde_mod_bsk_array = NULL;
    }
    if (self->coeff_products_all_mod_bsk_array) {
        hedge_free(self->coeff_products_all_mod_bsk_array);
        self->coeff_products_all_mod_bsk_array = NULL;
    }
    if (self->coeff_products_mod_plain_gamma_array) {
        for (i = 0; i < self->plain_gamma_count; i++) {
            hedge_free(self->coeff_products_mod_plain_gamma_array[i]);
        }
        hedge_free(self->coeff_products_mod_plain_gamma_array);
        self->coeff_products_mod_plain_gamma_array = NULL;
    }
    if (self->neg_inv_coeff_products_all_mod_plain_gamma_array) {
        hedge_free(self->neg_inv_coeff_products_all_mod_plain_gamma_array);
        self->neg_inv_coeff_products_all_mod_plain_gamma_array = NULL;
    }
    if (self->plain_gamma_product_mod_coeff_array) {
        hedge_free(self->plain_gamma_product_mod_coeff_array);
        self->plain_gamma_product_mod_coeff_array = NULL;
    }

    for (size_t i = 0; i < self->bsk_base_mod_count; i++) {
        if (self->bsk_small_ntt_tables[i]) {
            del_SmallNTTTables(self->bsk_small_ntt_tables[i]);
            self->bsk_small_ntt_tables[i] = NULL;
        }
    }
    hedge_free(self->bsk_small_ntt_tables);

    if (self->inv_last_coeff_mod_array) {
        hedge_free(self->inv_last_coeff_mod_array);
        self->inv_last_coeff_mod_array = NULL;
    }

    memset((void *)self, 0, sizeof(struct base_converter) - offsetof(struct base_converter, init));
}

static void bcfree(struct base_converter *self)
{
    self->reset(self);
}

static void fastbconv(
        struct base_converter *self,
        const uint64_t *input,
        uint64_t *dest)
{
    uint64_t *temp_coeff_transition;
    size_t i, j, k;

#ifdef HEDGE_DEBUG
    assert(input);
    assert(dest);
    assert(self->generated);
#endif
    /**
     * Require: Input in q
     * Ensure: Output in Bsk = {m1,...,ml} U {msk}
     */
    temp_coeff_transition =
            allocate_uint(self->coeff_count * self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        uint64_t inv_coeff_base_products_mod_coeff_elt
                = self->inv_coeff_base_products_mod_coeff_array[i];
        Zmodulus *coeff_base_array_elt = self->coeff_base_array[i];

        for (j = 0; j < self->coeff_count; j++, input++) {
            temp_coeff_transition[i + (j * self->coeff_base_mod_count)] =
                    multiply_uint_uint_smallmod(*input,
                            inv_coeff_base_products_mod_coeff_elt,
                            coeff_base_array_elt
                    );
        }
    }

    for (i = 0; i < self->bsk_base_mod_count; i++) {
        uint64_t *temp_coeff_transition_ptr = (uint64_t *)temp_coeff_transition;
        Zmodulus *bsk_base_array_elt = self->bsk_base_array[i];

        for (j = 0; j < self->coeff_count; j++, dest++) {
            uint64_t *coeff_base_products_mod_aux_bsk_array_ptr
                    = self->coeff_base_products_mod_aux_bsk_array[j];
            uint64_t aux_transition[2] = { 0, 0 };

            for (k = 0; k < self->coeff_base_mod_count;
                 k++, temp_coeff_transition_ptr++, coeff_base_products_mod_aux_bsk_array_ptr++) {
                uint64_t temp[2];
                unsigned char carry;

                /* Product is 60 bit + 61 bit = 121 bit, so can sum up to 127
                 * of them with no reduction.
                 * Thus need coeff_base_mod_count_ <= 127 to guarantee success */
                multiply_uint64(*temp_coeff_transition_ptr,
                        *coeff_base_products_mod_aux_bsk_array_ptr, temp);
                carry = add_uint64(aux_transition[0], temp[0], aux_transition);
                aux_transition[1] += temp[1] + carry;
            }
            *dest = barrett_reduce_128(aux_transition, bsk_base_array_elt);
        }
    }

    hedge_free(temp_coeff_transition);
}

static void floor_last_coeff_modulus_inplace(
        struct base_converter *self,
        uint64_t *rns_poly)
{
    size_t i;
    void *temp = allocate_uint(self->coeff_count);

    for (i = 0; i < self->coeff_base_mod_count - 1; i++) {
        /* (ct mod qk) mod qi */
        modulo_poly_coeffs_63(
                rns_poly + (self->coeff_base_mod_count - 1) * self->coeff_count,
                self->coeff_count,
                self->coeff_base_array[i],
                (uint64_t *)temp);
        sub_poly_poly_coeffsmallmod(
                rns_poly + i * self->coeff_count,
                (uint64_t *)temp,
                self->coeff_count,
                self->coeff_base_array[i],
                rns_poly + i * self->coeff_count);

        /* qk^(-1) * ((ct mod qi) - (ct mod qk)) mod qi */
        multiply_poly_scalar_coeffsmallmod(
                rns_poly + i * self->coeff_count,
                self->coeff_count,
                self->inv_last_coeff_mod_array[i],
                self->coeff_base_array[i],
                rns_poly + i * self->coeff_count);
    }

    hedge_free(temp);
}

static void floor_last_coeff_modulus_ntt_inplace(
        struct base_converter *self,
        uint64_t *rns_poly,
        const SmallNTTTables **rns_ntt_tables)
{
    size_t i;
    DEBUG_TRACE_LINE();
    void *temp = allocate_uint(self->coeff_count);
    log_fatal("temp=%p\n", temp);
    DEBUG_TRACE_LINE();
    /* Convert to non-NTT form */
    inverse_ntt_negacyclic_harvey(
            rns_poly + (self->coeff_base_mod_count - 1) * self->coeff_count,
            rns_ntt_tables[self->coeff_base_mod_count - 1]);
    DEBUG_TRACE_LINE();
    for (i = 0; i < self->coeff_base_mod_count - 1; i++) {
        DEBUG_TRACE_LINE();
        /* (ct mod qk) mod qi */
        modulo_poly_coeffs_63(
                rns_poly + (self->coeff_base_mod_count - 1) * self->coeff_count,
                self->coeff_count,
                self->coeff_base_array[i],
                (uint64_t *)temp);
        DEBUG_TRACE_LINE();
        /* Convert to NTT form */
        ntt_negacyclic_harvey((uint64_t *)temp, rns_ntt_tables[i]);
        DEBUG_TRACE_LINE();
        /* ((ct mod qi) - (ct mod qk)) mod qi */
        sub_poly_poly_coeffsmallmod(
                rns_poly + i * self->coeff_count,
                (uint64_t *)temp,
                self->coeff_count,
                self->coeff_base_array[i],
                rns_poly + i * self->coeff_count);
        DEBUG_TRACE_LINE();
        /* qk^(-1) * ((ct mod qi) - (ct mod qk)) mod qi */
        multiply_poly_scalar_coeffsmallmod(
                rns_poly + i * self->coeff_count,
                self->coeff_count,
                self->inv_last_coeff_mod_array[i],
                self->coeff_base_array[i],
                rns_poly + i * self->coeff_count);
        DEBUG_TRACE_LINE();
    }
    DEBUG_TRACE_LINE();
    log_fatal("free %p\n", temp);
    hedge_free(temp);
    DEBUG_TRACE_LINE();
}

static void fastbconv_sk(
        struct base_converter *self,
        const uint64_t *input,
        uint64_t *destination)
{
    size_t i, j, k;
    void *alpha_sk;
    const uint64_t *input_ptr = input;
    uint64_t *destination_ptr = destination;
    uint64_t *temp_ptr;
    const uint64_t m_sk_value = self->m_sk->value;
    const uint64_t m_sk_div_2 = m_sk_value >> 1;
    uint64_t *temp_coeff_transition = allocate_uint(self->coeff_count * self->aux_base_mod_count);
    void *tmp = allocate_uint(self->coeff_count);

#ifdef HEDGE_DEBUG
    assert(input);
    assert(destination);
#endif
    /**
     * Require: Input in base Bsk = M U {msk}
     * Ensure: Output in base q
     */
    for (i = 0; i < self->aux_base_mod_count; i++) {
        uint64_t inv_aux_base_products_mod_aux_array_elt =
                self->inv_aux_base_products_mod_aux_array[i];
        Zmodulus *aux_base_array_elt = self->aux_base_array[i];

        for (j = 0; j < self->coeff_count; j++) {
            temp_coeff_transition[i + (j * self->aux_base_mod_count)] =
                    multiply_uint_uint_smallmod(*input_ptr++,
                            inv_aux_base_products_mod_aux_array_elt,
                            aux_base_array_elt
                    );
        }
    }

    for (i = 0; i < self->coeff_base_mod_count; i++) {
        temp_ptr = (uint64_t *)temp_coeff_transition;
        Zmodulus *coeff_base_array_elt = self->coeff_base_array[i];

        for (j = 0; j < self->coeff_count; j++, destination_ptr++) {
            const uint64_t *aux_base_products_mod_coeff_array_ptr =
                    (uint64_t *)self->aux_base_products_mod_coeff_array[i];
            uint64_t aux_transition[2] = { 0, 0 };

            for (k = 0; k < self->aux_base_mod_count;
                 k++, temp_ptr++, aux_base_products_mod_coeff_array_ptr++) {
                uint64_t temp[2];
                unsigned char carry;

                /* Product is 61 bit + 60 bit = 121 bit, so can sum up to 127 of them with no reduction
                 * Thus need aux_base_mod_count_ <= 127, so coeff_base_mod_count_ <= 126 to guarantee success */
                multiply_uint64(*temp_ptr, *aux_base_products_mod_coeff_array_ptr, temp);
                carry = add_uint64(aux_transition[0], temp[0], aux_transition);
                aux_transition[1] += temp[1] + carry;
            }
            *destination_ptr = barrett_reduce_128(aux_transition, coeff_base_array_elt);
        }
    }

    /* Compute alpha_sk
     * Require: Input is in Bsk
     * we only use coefficient in B
     * Fast convert B -> m_sk */
    destination_ptr = (uint64_t *)tmp;
    temp_ptr = (uint64_t *)temp_coeff_transition;
    for (i = 0; i < self->coeff_count; i++, destination_ptr++) {
        uint64_t msk_transition[2] = { 0, 0 };
        const uint64_t *aux_base_products_mod_msk_array_ptr =
                (uint64_t *)self->aux_base_products_mod_msk_array;

        for (j = 0; j < self->aux_base_mod_count;
             j++, temp_ptr++, aux_base_products_mod_msk_array_ptr++) {
            uint64_t temp[2];
            unsigned char carry;

            /* Product is 61 bit + 61 bit = 122 bit, so can sum up to 63 of them with no reduction
             * Thus need aux_base_mod_count_ <= 63, so coeff_base_mod_count_ <= 62 to guarantee success
             * This gives the strongest restriction on the number of coeff modulus primes */
            multiply_uint64(*temp_ptr, *aux_base_products_mod_msk_array_ptr, temp);
            carry = add_uint64(msk_transition[0], temp[0], msk_transition);
            msk_transition[1] += temp[1] + carry;
        }
        *destination_ptr = barrett_reduce_128(msk_transition, self->m_sk);
    }

    alpha_sk = allocate_uint(self->coeff_count);
    input_ptr = input + (self->aux_base_mod_count * self->coeff_count);
    destination_ptr = (uint64_t *)alpha_sk;
    temp_ptr = (uint64_t *)tmp;

    /* x_sk is allocated in input[aux_base_mod_count] */
    for (i = 0; i < self->coeff_count;
         i++, input_ptr++, temp_ptr++, destination_ptr++) {
        /* It is not necessary for the negation to be reduced modulo the small prime */
        uint64_t negated_input = m_sk_value - *input_ptr;
        *destination_ptr = multiply_uint_uint_smallmod(*temp_ptr + negated_input,
                self->inv_aux_products_mod_msk, self->m_sk);
    }

    destination_ptr = destination;
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        uint64_t coeff_base_array_elt_value;
        uint64_t aux_products_all_mod_coeff_array_elt = self->aux_products_all_mod_coeff_array[i];
        temp_ptr = (uint64_t *)alpha_sk;
        Zmodulus *coeff_base_array_elt = self->coeff_base_array[i];
        coeff_base_array_elt_value = coeff_base_array_elt->value;

        for (k = 0; k < self->coeff_count;
             k++, temp_ptr++, destination_ptr++) {
            uint64_t m_alpha_sk[2];

            /* Correcting alpha_sk since it is a centered modulo */
            if (*temp_ptr > m_sk_div_2) {
                multiply_uint64(aux_products_all_mod_coeff_array_elt,
                        m_sk_value - *temp_ptr, m_alpha_sk);
                m_alpha_sk[1] += add_uint64(m_alpha_sk[0], *destination_ptr, m_alpha_sk);
                *destination_ptr = barrett_reduce_128(m_alpha_sk, coeff_base_array_elt);
            } else {
                /* It is not necessary for the negation to be reduced modulo the small prime */
                multiply_uint64(
                        coeff_base_array_elt_value - aux_products_all_mod_coeff_array_elt,
                        *temp_ptr, m_alpha_sk);
                m_alpha_sk[1] += add_uint64(*destination_ptr, m_alpha_sk[0], m_alpha_sk);
                *destination_ptr = barrett_reduce_128(m_alpha_sk, coeff_base_array_elt);
            }
        }
    }

    hedge_free(temp_coeff_transition);
    hedge_free(tmp);
    hedge_free(alpha_sk);
}

static void mont_rq(
        struct base_converter *self,
        const uint64_t *input,
        uint64_t *destination)
{
    size_t i, j;
    uint64_t *input_m_tilde_ptr
            = (uint64_t *)input + self->coeff_count * self->bsk_base_mod_count;

#ifdef HEDGE_DEBUG
    assert(input);
    assert(destination);
#endif
    /**
     * Require: Input should in Bsk U {m_tilde}
     * Ensure: Destination array in Bsk = m U {msk}
     */
    for (i = 0; i < self->bsk_base_mod_count; i++) {
        uint64_t coeff_products_all_mod_bsk_array_elt
                = self->coeff_products_all_mod_bsk_array[i];
        uint64_t inv_mtilde_mod_bsk_array_elt
                = self->inv_mtilde_mod_bsk_array[i];
        const uint64_t *input_m_tilde_ptr_copy = input_m_tilde_ptr;
        Zmodulus *bsk_base_array_elt = self->bsk_base_array[i];

        /* Compute result for aux base */
        for (j = 0; j < self->coeff_count;
             j++, destination++, input_m_tilde_ptr_copy++, input++) {
            uint64_t tmp[2];                 
            /* Compute r_mtilde
             * Duplicate work here:
             * This needs to be computed only once per coefficient, not per Bsk prime. */
            uint64_t r_mtilde = multiply_uint_uint_smallmod(*input_m_tilde_ptr_copy,
                                        self->inv_coeff_products_mod_mtilde, self->m_tilde);
            r_mtilde = negate_uint_smallmod(r_mtilde, self->m_tilde);

            /* Lazy reduction */
            multiply_uint64(coeff_products_all_mod_bsk_array_elt, r_mtilde, tmp);
            tmp[1] += add_uint64(tmp[0], *input, tmp);
            r_mtilde = barrett_reduce_128(tmp, bsk_base_array_elt);
            *destination = multiply_uint_uint_smallmod(
                    r_mtilde, inv_mtilde_mod_bsk_array_elt, bsk_base_array_elt);
        }
    }
}

static void fast_floor(
        struct base_converter *self,
        const uint64_t *input,
        uint64_t *destination)
{
    size_t i, j;
    size_t index_msk = self->coeff_base_mod_count * self->coeff_count;

#ifdef HEDGE_DEBUG
    assert(input);
    assert(destination);
#endif
    /**
     * Require: Input in q U m U {msk}
     * Ensure: Destination array in Bsk
     */
    self->fastbconv(self, input, destination);
    input += index_msk;
    for (i = 0; i < self->bsk_base_mod_count; i++) {
        uint64_t bsk_base_array_value, inv_coeff_products_all_mod_aux_bsk_array_elt;
        Zmodulus *bsk_base_array_elt = self->bsk_base_array[i];
        bsk_base_array_value = bsk_base_array_elt->value;
        inv_coeff_products_all_mod_aux_bsk_array_elt
                = self->inv_coeff_products_all_mod_aux_bsk_array[i];

        for (j = 0; j < self->coeff_count;
             j++, input++, destination++) {
            /* It is not necessary for the negation to be reduced modulo the small prime
             * negate_uint_smallmod(base_convert_Bsk.get() + k + (i * coeff_count_),
             * bsk_base_array_[i], &negated_base_convert_Bsk); */
            *destination = multiply_uint_uint_smallmod(
                    *input + bsk_base_array_value - *destination,
                    inv_coeff_products_all_mod_aux_bsk_array_elt,
                    bsk_base_array_elt
            );
        }
    }
}

static void fastbconv_mtilde(
        struct base_converter *self,
        const uint64_t *input,
        uint64_t *destination)
{
    size_t i, j, k;
    uint64_t *temp_coeff_transition;
    uint64_t *temp_coeff_transition_ptr;
    uint64_t *destination_ptr = destination;

#ifdef HEDGE_DEBUG
    assert(input);
    assert(destination);
#endif
    /**
     * Require: Input in q
     * Ensure: Output in Bsk U {m_tilde}
     */

    /* Compute in Bsk first; we compute |m_tilde*q^-1i| mod qi */
    temp_coeff_transition
            = allocate_uint(self->coeff_count * self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        uint64_t mtilde_inv_coeff_base_products_mod_coeff_elt
                = self->mtilde_inv_coeff_base_products_mod_coeff_array[i];
        Zmodulus *coeff_base_array_elt = self->bsk_base_array[i];

        for (j = 0; j < self->coeff_count; j++, input++) {
            temp_coeff_transition[i + (j * self->coeff_base_mod_count)] =
                    multiply_uint_uint_smallmod(*input,
                            mtilde_inv_coeff_base_products_mod_coeff_elt,
                            coeff_base_array_elt
                    );
        }
    }

    for (i = 0; i < self->bsk_base_mod_count; i++) {
        const uint64_t *coeff_base_products_mod_aux_bsk_array_ptr
                = self->coeff_base_products_mod_aux_bsk_array[i];
        uint64_t *temp_coeff_transition_ptr = (uint64_t *)temp_coeff_transition;
        Zmodulus *bsk_base_array_elt = self->bsk_base_array[i];

        for (j = 0; j < self->coeff_count; j++, destination_ptr++) {
            uint64_t aux_transition[2] = { 0, 0 };
            const uint64_t *temp_ptr = coeff_base_products_mod_aux_bsk_array_ptr;

            for (k = 0; k < self->coeff_base_mod_count;
                 k++, temp_ptr++, temp_coeff_transition_ptr++) {
                unsigned char carry;
                uint64_t temp[2] = { 0, 0 };

                /* Product is 60 bit + 61 bit = 121 bit, so can sum up to 127 of them with no reduction
                 * Thus need coeff_base_mod_count_ <= 127 */
                multiply_uint64(*temp_coeff_transition_ptr, *temp_ptr, temp);
                carry = add_uint64(aux_transition[0], temp[0], aux_transition);
                aux_transition[1] += temp[1] + carry;
            }
            *destination_ptr = barrett_reduce_128(aux_transition, bsk_base_array_elt);
        }
    }

    /* Computing the last element (mod m_tilde) and add it at the end of destination array */
    temp_coeff_transition_ptr = (uint64_t *)temp_coeff_transition;
    destination += self->bsk_base_mod_count * self->coeff_count;
    for (i = 0; i < self->coeff_count; i++, destination++) {
        uint64_t wide_result[2] = { 0, 0 };
        const uint64_t *coeff_base_products_mod_mtilde_array_ptr
                = (const uint64_t *)self->coeff_base_products_mod_mtilde_array;

        for (j = 0; j < self->coeff_base_mod_count;
             j++, temp_coeff_transition_ptr++, coeff_base_products_mod_mtilde_array_ptr++) {
            unsigned char carry;
            uint64_t aux_transition[2];

            /* Product is 60 bit + 33 bit = 93 bit */
            multiply_uint64(*temp_coeff_transition_ptr,
                    *coeff_base_products_mod_mtilde_array_ptr, aux_transition);
            carry = add_uint64(aux_transition[0], wide_result[0], wide_result);
            wide_result[1] += aux_transition[1] + carry;
        }
        *destination = barrett_reduce_128(wide_result, self->m_tilde);
    }
    hedge_free(temp_coeff_transition);
}

static void fastbconv_plain_gamma(
        struct base_converter *self,
        const uint64_t *input,
        uint64_t *destination)
{
    size_t i, j, k;
    uint64_t *temp_coeff_transition;
#ifdef HEDGE_DEBUG
    assert(self->small_plain_mod->value > 0);
    assert(input);
    assert(destination);
#endif
    /**
     * Require: Input in q
     * Ensure: Output in t (plain modulus) U gamma
     */
    temp_coeff_transition
            = allocate_uint(self->coeff_count * self->coeff_base_mod_count);
    for (i = 0; i < self->coeff_base_mod_count; i++) {
        uint64_t inv_coeff_base_products_mod_coeff_elt
                = self->inv_coeff_base_products_mod_coeff_array[i];
        Zmodulus *coeff_base_array_elt = self->coeff_base_array[i];

        for (j = 0; j < self->coeff_count; j++, input++) {
            temp_coeff_transition[i + (j * self->coeff_base_mod_count)]
                    = multiply_uint_uint_smallmod(*input,
                            inv_coeff_base_products_mod_coeff_elt,
                            coeff_base_array_elt
                    );
        }
    }

    for (i = 0; i < self->plain_gamma_count; i++) {
        uint64_t *temp_coeff_transition_ptr = (uint64_t *)temp_coeff_transition;
        const uint64_t *coeff_products_mod_plain_gamma_array_ptr
                = (const uint64_t *)self->coeff_products_mod_plain_gamma_array[i];
        Zmodulus *plain_gamma_array_elt = self->plain_gamma_array[i];

        for (j = 0; j < self->coeff_count; j++, destination++) {
            uint64_t wide_result[2] = { 0, 0 };
            const uint64_t *temp_ptr = coeff_products_mod_plain_gamma_array_ptr;

            for (k = 0; k < self->coeff_base_mod_count;
                 k++, temp_coeff_transition_ptr++, temp_ptr++) {
                unsigned char carry;
                uint64_t plain_transition[2];

                /* Lazy reduction
                 * Product is 60 bit + 61 bit = 121 bit, so can sum up to 127 of them with no reduction
                 * Thus need coeff_base_mod_count_ <= 127 */
                multiply_uint64(*temp_coeff_transition_ptr, *temp_ptr, plain_transition);
                carry = add_uint64(plain_transition[0], wide_result[0], wide_result);
                wide_result[1] += plain_transition[1] + carry;
            }
            *destination = barrett_reduce_128(wide_result, plain_gamma_array_elt);
        }
    }
    hedge_free(temp_coeff_transition);    
}

struct base_converter *new_baseconverter(void)
{
    struct base_converter *inst = hedge_zalloc(sizeof(struct base_converter));
    if (!inst)
        return NULL;
    log_trace("new_baseconverter = %p\n", inst);
    *inst = (struct base_converter) {
        .init = init,
        .init_with_parms = init_with_parms,
        //.assign = assign,
        .free = bcfree,
        .generate = generate_baseconverter,
        .reset = reset,
        .floor_last_coeff_modulus_inplace = floor_last_coeff_modulus_inplace,
        .floor_last_coeff_modulus_ntt_inplace = floor_last_coeff_modulus_ntt_inplace,
        .fastbconv = fastbconv,
        .fastbconv_sk = fastbconv_sk,
        .mont_rq = mont_rq,
        .fast_floor = fast_floor,
        .fastbconv_mtilde = fastbconv_mtilde,
        .fastbconv_plain_gamma = fastbconv_plain_gamma
    };

    inst->init(inst);
    return inst;
}

void del_baseconverter(struct base_converter *inst)
{
    if (inst->free)
        inst->free(inst);
    hedge_free(inst);
}

#ifdef __cplusplus
}
#endif