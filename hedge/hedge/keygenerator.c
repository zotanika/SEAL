#include <assert.h>

#include "keygenerator.h"
#include "valcheck.h"
#include "context.h"
#include "math/uintcore.h"
#include "math/rng.h"
#include "math/rlwe.h"
#include "math/polycore.h"
#include "math/polyarithsmallmod.h"

static void init_keygen(
        struct key_generator *self,
        struct hedge_context *ctx)
{
    assert(ctx);
    assert(ctx->is_parms_set(ctx));

    self->context = ctx;
    self->sk_generated = false;
    self->pk_generated = false;
    
    self->generate_secretkey(self, false);
    self->generate_publickey(self);
}

static void free_keygen(struct key_generator *self)
{
    if (self->public_key) {
        del_PublicKey(self->public_key);
        self->public_key = NULL;
    }
    if (self->secret_key) {
        del_SecretKey(self->secret_key);
        self->secret_key =  NULL;
    }
    if (self->secret_key_array) {
        hedge_free(self->secret_key_array);
        self->secret_key_array = NULL;
    }
    if (self->generated_relinkeys) {
        del_relinkeys(self->generated_relinkeys);
        self->generated_relinkeys = NULL;
    }
}

static void init_with_skey(
                struct key_generator *self,
                struct hedge_context *ctx,
                const SecretKey *skey)
{
    assert(ctx);
    assert(ctx->is_parms_set(ctx));
    assert(is_valid_for(SecretKey, skey, ctx));

    self->secret_key = (SecretKey *)skey;
    self->sk_generated = true;

    self->generate_secretkey(self, self->sk_generated);
    self->generate_publickey(self);
}

static void init_with_skeynpkey(
                struct key_generator *self,
                struct hedge_context *ctx,
                const SecretKey *skey,
                const PublicKey *pkey)
{
    assert(ctx);
    assert(ctx->is_parms_set(ctx));
    assert(is_valid_for(SecretKey, skey, ctx));
    assert(is_valid_for(PublicKey, pkey, ctx));

    self->secret_key = (SecretKey *)skey;
    self->public_key = (PublicKey *)pkey;
    self->sk_generated = true;
    self->pk_generated = true;

    self->generate_secretkey(self, self->sk_generated);
    self->generate_publickey(self); 
}

static void generate_secretkey(
                struct key_generator *self,
                bool is_initialized)
{
    size_t i;

    Plaintext *indata;
    struct rng random;
    uint64_t *raw_coeffs;

    ctxdata_t *context_data = self->context->key_ctxdata(self->context);
    encrypt_parameters *parms = context_data->parms;
    vector(Zmodulus) *coeff_modulus = parms->coeff_modulus(parms);
    size_t coeff_count = parms->poly_modulus_degree(parms);
    size_t coeff_mod_count = coeff_modulus->size;
    log_trace("@@@ generate_secretkey() context_data = %p\n", context_data);
    if (!is_initialized) {
        self->secret_key = new_SecretKey();
        self->sk_generated = false;
        indata = self->secret_key->data(self->secret_key);
        indata->resize(indata, mul_safe(coeff_count, coeff_mod_count));

        rng_init_uniform_default(&random);
        raw_coeffs = indata->data(indata);
        sample_poly_ternary(raw_coeffs, &random, (const encrypt_parameters *)parms);

        for (i = 0; i < coeff_mod_count; i++) {
            ntt_negacyclic_harvey(raw_coeffs + (i * coeff_count),
                    context_data->small_ntt_tables[i]);
        }
        self->secret_key->sk_->parms_id_ = context_data->parms->parms_id_;
        print_array_trace( self->secret_key->sk_->parms_id_.data, 4);
        print_array_trace( context_data->parms->parms_id_.data, 4);
        
    }

    self->secret_key_array = allocate_poly(coeff_count, coeff_mod_count);

    indata = self->secret_key->data(self->secret_key);
    raw_coeffs = indata->data(indata);
    set_poly_poly_simple(raw_coeffs, coeff_count, coeff_mod_count, self->secret_key_array);
    self->secret_key_array_size = 1;

    self->sk_generated = true;
}

static void generate_publickey(struct key_generator *self)
{
    ctxdata_t *context_data;
    encrypt_parameters *parms;
    vector(Zmodulus) *coeff_modulus;
    size_t coeff_count;
    size_t coeff_mod_count;
    struct rng random;    

    assert(self->sk_generated);

    context_data = self->context->key_ctxdata(self->context);
    parms = context_data->parms;
    coeff_modulus = parms->coeff_modulus(parms);
    coeff_count = parms->poly_modulus_degree(parms);
    coeff_mod_count = coeff_modulus->size;

    assert(product_fits_in(coeff_count, coeff_mod_count));

    /* Initialize public key.
     * PublicKey data allocated from pool given by MemoryManager::GetPool. */
    self->public_key = new_PublicKey();
    self->pk_generated = false;

    rng_init_uniform_default(&random);
    encrypt_zero_symmetric(self->secret_key, self->context, parms->parms_id(parms),
            &random, true, self->public_key->pk_);

    //print_polynomial("pubkey", self->public_key->pk_->data_->data, self->public_key->pk_->size_, TRUE);

    self->public_key->pk_->parms_id_ = *parms->parms_id(parms);
    self->pk_generated = true;
    log_trace("@@@ public_key=%p\n", self->public_key);
}

static relinkeys_t *relin_keys(struct key_generator *self, size_t count)
{
    ctxdata_t *context_data;
    encrypt_parameters *parms;
    vector(Zmodulus) *coeff_modulus;
    size_t coeff_count;
    size_t coeff_mod_count;
    struct rng random;

    assert(self->sk_generated);
    assert(count || count > HEDGE_CIPHERTEXT_SIZE_MAX - 2);

    context_data = self->context->key_ctxdata(self->context);
    parms = context_data->parms;
    coeff_modulus = parms->coeff_modulus(parms);
    coeff_count = parms->poly_modulus_degree(parms);
    coeff_mod_count = coeff_modulus->size;

    assert(product_fits_in(coeff_count, coeff_mod_count));

    rng_init_uniform_default(&random);
    self->compute_secret_key_array(self, context_data, count + 1);

    self->generated_relinkeys = new_relinkeys();

    self->generate_kswitchkeys(self,
            (const uint64_t *)self->secret_key_array + (coeff_mod_count * coeff_count),
            count,
            self->generated_relinkeys->parent);
            //(struct kswitch_keys *)container_of(relin_keys, relinkeys_t, parent));

    self->generated_relinkeys->parent->parms_id = parms->parms_id(parms);
    return self->generated_relinkeys;
}

static galois_keys_t *
galois_keys_by_elts(struct key_generator *self, const vector(uint64_t) *galois_elts)
{
    ctxdata_t *context_data;
    encrypt_parameters *parms;
    vector(Zmodulus) *coeff_modulus;
    size_t coeff_count;
    size_t coeff_mod_count;
    int coeff_count_power;
    galois_keys_t *galois_keys;
    vector(vector_publickey) *pkeys;
    size_t i, j;

    assert(self->sk_generated);

    context_data = self->context->key_ctxdata(self->context);
    parms = context_data->parms;
    coeff_modulus = parms->coeff_modulus(parms);
    coeff_count = parms->poly_modulus_degree(parms);
    coeff_mod_count = coeff_modulus->size;
    coeff_count_power = get_power_of_two(coeff_count);

    assert(product_fits_in(coeff_count, coeff_mod_count, (size_t)2));

    galois_keys = new_galoiskeys();
    pkeys = galois_keys->parent->keys;
    pkeys->resize(pkeys, coeff_count);

    for (i = 0; i < galois_elts->size; i++) {
        uint64_t galois_elt = galois_elts->value_at((vector(uint64_t) *)galois_elts, i);
        uint64_t *rotated_secret_key = allocate_poly(coeff_count, coeff_mod_count);
        uint64_t index;
        vector(vector_publickey) *this;

        assert(galois_elt || (galois_elt >= 2 * coeff_count));

        if (galois_keys->has_key(galois_keys, galois_elt))
            continue;

        for (j = 0; j < coeff_mod_count; j++) {
            Plaintext *in = self->secret_key->data(self->secret_key);
            apply_galois_ntt((uint64_t *)in->data(in) + j * coeff_count,
                    coeff_count_power,
                    galois_elt,
                    rotated_secret_key + j * coeff_count);
        }

        /* Initialize Galois key
         * This is the location in the galois_keys vector */        
        index = galois_keys->get_index(galois_elt);

        /* Create Galois keys. */
        this = galois_keys->parent->keys;
        self->generate_one_kswitchkey(self,
                rotated_secret_key,
                this->at(this, index));
    }

    galois_keys->parent->parms_id = &context_data->parms->parms_id_;
    return galois_keys;    
}

static galois_keys_t *
galois_keys_by_step(struct key_generator *self, const vector(int) *steps)
{
    ctxdata_t *context_data;
    encrypt_parameters *parms;
    vector(Zmodulus) *coeff_modulus;
    size_t coeff_count;
    vector(uint64_t) *galois_elts;
    size_t i;
    
    size_t coeff_mod_count;
    int coeff_count_power;
    galois_keys_t *galois_keys;
    vector(vector_publickey) *pkeys;

    assert(self->sk_generated);

    context_data = self->context->key_ctxdata(self->context);
    parms = context_data->parms;

    assert(context_data->qualifiers->using_batching);

    coeff_count = parms->poly_modulus_degree(parms);
    galois_elts = new_vector(uint64_t, 0);
    for (i = 0; i < steps->size; i++) {
        galois_elts->push_back(galois_elts,
                            steps_to_galois_elt(steps->value_at((vector(int) *)steps, i), coeff_count));
    }
    return self->galois_keys_by_elts(self, galois_elts);
}

static galois_keys_t *
galois_keys(struct key_generator *self)
{
    ctxdata_t *context_data;
    encrypt_parameters *parms;
    size_t i;
    size_t coeff_count;
    uint64_t m;
    int logn;
    uint64_t two_power_of_three;
    uint64_t neg_two_power_of_three;
    vector(uint64_t) *logn_galois_keys;

    assert(self->sk_generated);

    context_data = self->context->key_ctxdata(self->context);
    parms = context_data->parms;
    coeff_count = parms->poly_modulus_degree(parms);
    m = coeff_count << 1;
    logn = get_power_of_two((uint64_t)coeff_count);

    logn_galois_keys = new_vector(uint64_t, 0);

    /* Generate Galois keys for m - 1 (X -> X^{m-1}) */
    logn_galois_keys->push_back(logn_galois_keys, m - 1);

    /* Generate Galois key for power of 3 mod m (X -> X^{3^k}) and
     * for negative power of 3 mod m (X -> X^{-3^k}) */
    two_power_of_three = 3;
    neg_two_power_of_three = 0;

    try_mod_inverse(3, m, &neg_two_power_of_three);
    for (i = 0; i < logn - 1; i++) {
        logn_galois_keys->push_back(logn_galois_keys, two_power_of_three);
        two_power_of_three *= two_power_of_three;
        two_power_of_three &= (m - 1);

        logn_galois_keys->push_back(logn_galois_keys, neg_two_power_of_three);
        neg_two_power_of_three *= neg_two_power_of_three;
        neg_two_power_of_three &= (m - 1);
    }
    return self->galois_keys_by_elts(self, logn_galois_keys);
}

static SecretKey *get_secret_key(struct key_generator *self)
{
    assert(self->sk_generated);
    return self->secret_key;
}

static PublicKey *get_public_key(struct key_generator *self)
{
    assert(self->pk_generated);
    return self->public_key;
}

static void keygen_compute_secret_key_array(
                struct key_generator *self,
                struct ctxdata *context_data,
                size_t max_power)
{
    encrypt_parameters *parms;
    vector(Zmodulus) *coeff_modulus;
    size_t coeff_count;
    size_t coeff_mod_count;
    size_t old_size, new_size;
    uint64_t *new_secret_key_array;
    size_t poly_ptr_increment;
    uint64_t *prev_poly_ptr;
    uint64_t *next_poly_ptr; 
    size_t i, j;

#ifdef HEDGE_DEBUG
    assert(max_power >= 1);
    assert(self->secret_key_array_size && self->secret_key_array);
#endif
    /* Extract encryption parameters. */
    parms = context_data->parms;
    coeff_modulus = parms->coeff_modulus(parms);
    coeff_count = parms->poly_modulus_degree(parms);
    coeff_mod_count = coeff_modulus->size;

    assert(product_fits_in(coeff_count, coeff_mod_count, max_power));

    old_size = self->secret_key_array_size;
    new_size = max(max_power, old_size);

    if (old_size == new_size)
        return;

    /* Need to extend the array
     * Compute powers of secret key until max_power */
    new_secret_key_array = allocate_poly(new_size * coeff_count, coeff_mod_count);
    set_poly_poly_simple(self->secret_key_array,
            old_size * coeff_count,
            coeff_mod_count,
            new_secret_key_array);

    poly_ptr_increment = coeff_count * coeff_mod_count;
    prev_poly_ptr = new_secret_key_array + (old_size - 1) * poly_ptr_increment;
    next_poly_ptr = prev_poly_ptr + poly_ptr_increment;

    /* Since all of the key powers in secret_key_array_ are already
     * NTT transformed, to get the next one we simply need to compute
     * a dyadic product of the last one with the first one
     * [which is equal to NTT(secret_key_)]. */
    for (i = old_size; i < new_size; i++) {
        for (j = 0; j < coeff_mod_count; j++) {
            dyadic_product_coeffsmallmod(prev_poly_ptr + (j * coeff_count),
                    new_secret_key_array + (j * coeff_count),
                    coeff_count, coeff_modulus->at(coeff_modulus, j),
                    next_poly_ptr + (j * coeff_count));
        }
        prev_poly_ptr = next_poly_ptr;
        next_poly_ptr += poly_ptr_increment;
    }

    /* Do we still need to update size? */
    old_size = self->secret_key_array_size;
    new_size = max(max_power, self->secret_key_array_size);

    if (old_size == new_size)
        return;

    if (self->secret_key_array)
        hedge_free(self->secret_key_array);
    self->secret_key_array_size = new_size;
    self->secret_key_array = new_secret_key_array;
}

static void keygen_generate_one_kswitchkey(
                struct key_generator *self,
                const uint64_t *new_key,
                vector_publickey *dest)
{
    ctxdata_t *context_data = self->context->key_ctxdata(self->context);
    encrypt_parameters *parms = context_data->parms;
    vector(Zmodulus) *key_modulus = parms->coeff_modulus(parms);
    size_t coeff_count = parms->poly_modulus_degree(parms);
    size_t decomp_mod_count = key_modulus->size;
    uint64_t *temp;
    uint64_t factor = 0;
    struct rng random;
    size_t j;

    assert(product_fits_in(coeff_count, decomp_mod_count));

    dest->resize(dest, decomp_mod_count);

    temp = allocate_uint(coeff_count);
    rng_init_uniform_default(&random);

    for (j = 0; j < decomp_mod_count; j++) {
        PublicKey *this = dest->at(dest, j);
        /** let's consider a better way */
        build_publickey(this);
        Ciphertext *that = this->data(this);
        encrypt_zero_symmetric(self->secret_key,
                self->context,
                parms->parms_id(parms),
                &random, true,
                this->data(this));

        factor = key_modulus->back(key_modulus)->value % key_modulus->at(key_modulus, j)->value;
        multiply_poly_scalar_coeffsmallmod(
                new_key + j * coeff_count,
                coeff_count,
                factor,
                key_modulus->at(key_modulus, j),
                temp);
        add_poly_poly_coeffsmallmod(
                that->data(that) + j * coeff_count,
                temp,
                coeff_count,
                key_modulus->at(key_modulus, j),
                that->data(that) + j * coeff_count);
    }
    hedge_free(temp);
}

static void keygen_generate_kswitchkeys(
                struct key_generator *self,
                const uint64_t *new_keys,
                size_t num_keys,
                struct kswitch_keys *dest)
{
    ctxdata_t *context_data = self->context->key_ctxdata(self->context);
    encrypt_parameters *parms = context_data->parms;
    size_t coeff_count = parms->poly_modulus_degree(parms);
    vector(Zmodulus) *coeff_modulus = parms->coeff_modulus(parms);
    size_t coeff_mod_count = coeff_modulus->size;
    size_t l;

    assert(product_fits_in(coeff_count, coeff_mod_count, num_keys));

    dest->keys->resize(dest->keys, num_keys);
    
    for (l = 0; l < num_keys; l++) {
        const uint64_t *new_key_ptr = new_keys + (l * coeff_mod_count * coeff_count);
        self->generate_one_kswitchkey(self, new_key_ptr, dest->keys->at(dest->keys, l));
    }
}

struct key_generator *new_keygenerator(struct hedge_context *context)
{
    struct key_generator *inst = hedge_zalloc(sizeof(struct key_generator));
    if (!inst)
        return NULL;

    *inst = (struct key_generator) {
        .init = init_keygen,
        .free = free_keygen,
        .init_with_skey = init_with_skey,
        .init_with_skeynpkey = init_with_skeynpkey,
        .get_secret_key = get_secret_key,
        .get_public_key = get_public_key,
        .generate_secretkey = generate_secretkey,
        .generate_publickey = generate_publickey,
        .generate_kswitchkeys = keygen_generate_kswitchkeys,
        .generate_one_kswitchkey = keygen_generate_one_kswitchkey,
        .compute_secret_key_array = keygen_compute_secret_key_array, 
        .relin_keys = relin_keys,
        .galois_keys_by_elts = galois_keys_by_elts,
        .galois_keys_by_step = galois_keys_by_step,
        .galois_keys = galois_keys
    };
    inst->init(inst, context);
    return inst;
}

void del_keygenerator(struct key_generator *self)
{
    if (self->free)
        self->free(self);
    hedge_free(self);
}