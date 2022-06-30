
#include "math/rlwe.h"
#include "lib/common.h"
#include "math/polycore.h"
#include "math/smallntt.h"
#include "globals.h"
#include "math/rng.h"
#include "math/polyarithsmallmod.h"

void sample_poly_ternary(
    uint64_t *poly,
    struct rng* random,
    const encrypt_parameters* parms)
{
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus_;
    size_t coeff_mod_count = coeff_modulus->size;
    size_t coeff_count = parms->poly_modulus_degree_;

    for (size_t i = 0; i < coeff_count; i++)
    {
        int rand_index = rng_get_rounded_int(random);
        if (rand_index == 1)
        {
            for (size_t j = 0; j < coeff_mod_count; j++)
            {
                poly[i + j * coeff_count] = 1;
            }
        }
        else if (rand_index == -1)
        {
            for (size_t j = 0; j < coeff_mod_count; j++)
            {
                Zmodulus* sm = coeff_modulus->at(coeff_modulus, j);
                poly[i + j * coeff_count] = sm->value - 1;
            }
        }
        else
        {
            for (size_t j = 0; j < coeff_mod_count; j++)
            {
                poly[i + j * coeff_count] = 0;
            }
        }
    }
}

void sample_poly_normal(
    uint64_t *poly,
    struct rng* random,
    const encrypt_parameters* parms)
{
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus_;
    size_t coeff_mod_count = coeff_modulus->size;
    size_t coeff_count = parms->poly_modulus_degree_;

    if (are_close(noise_max_deviation, 0.0))
    {
        set_zero_poly(coeff_count, coeff_mod_count, poly);
        return;
    }

    struct rng rng;
    rng_init(&rng, rng_get_seed(), 0, noise_standard_deviation);

    for (size_t i = 0; i < coeff_count; i++)
    {
        int64_t noise = (int64_t)rng_get_maxdev(&rng, noise_max_deviation);
        if (noise > 0)
        {
            for (size_t j = 0; j < coeff_mod_count; j++)
            {
                poly[i + j * coeff_count] =(uint64_t)(noise);
            }
        }
        else if (noise < 0)
        {
            noise = -noise;
            for (size_t j = 0; j < coeff_mod_count; j++)
            {
                Zmodulus* sm = coeff_modulus->at(coeff_modulus, j);
                poly[i + j * coeff_count] = sm->value - (uint64_t)(noise);
            }
        }
        else
        {
            for (size_t j = 0; j < coeff_mod_count; j++)
            {
                poly[i + j * coeff_count] = 0;
            }
        }
    }
}

void sample_poly_uniform(
    uint64_t *poly,
    struct rng* random,
    const encrypt_parameters* parms)
{
    // Extract encryption parameters.
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus((encrypt_parameters*)parms);
    size_t coeff_mod_count = coeff_modulus->size;
    size_t coeff_count = parms->poly_modulus_degree((encrypt_parameters*)parms);

    uint64_t max_uint64 = ULONG_MAX;
    uint64_t modulus = 0;
    uint64_t max_multiple = 0;

    for (size_t j = 0; j < coeff_mod_count; j++)
    {
        Zmodulus* sm = coeff_modulus->at(coeff_modulus, j);
        modulus = sm->value;
        max_multiple = max_uint64 - max_uint64 % modulus;
        for (size_t i = 0; i < coeff_count; i++)
        {
            // This ensures uniform distribution.
            uint64_t rand;
            do
            {
                rand = (((uint64_t)(rng_get(random)) << 32) +
                    (uint64_t)(rng_get(random)));
            }
            while (rand >= max_multiple);
            poly[i + j * coeff_count] = rand % modulus;
        }
    }
}

void encrypt_zero_asymmetric(
    const PublicKey* public_key,
    hedge_context_t* context,
    parms_id_type* parms_id,
    struct rng* random,
    bool is_ntt_form,
    Ciphertext* destination)
{
    if (!EQUALS(public_key->parms_id((PublicKey*)public_key), &context->key_parms_id))
    {
        log_trace("encrypt_zero_asymmetric(%p, %p,.)\n", public_key, context);
        print_array_info(public_key->pk_->parms_id_.data, 4);
        print_array_info(context->key_parms_id.data, 4);
        
        invalid_argument("key_parms_id mismatch");
    }

    ctxdata_t* context_data = context->get_ctxdata(context, parms_id);
    encrypt_parameters* parms = context_data->parms;
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus(parms);
    size_t coeff_mod_count = coeff_modulus->size;
    size_t coeff_count = parms->poly_modulus_degree(parms);
    SmallNTTTables** small_ntt_tables = context_data->small_ntt_tables;
    Ciphertext* pubkey_data = public_key->data((PublicKey*)public_key);
    size_t encrypted_size = pubkey_data->size(pubkey_data);

    if (encrypted_size < 2)
    {
        invalid_argument("public_key has less than 2 parts");
    }

    // Make destination have right size and parms_id
    // Ciphertext (c_0,c_1, ...)
    destination->resize(destination, context, parms_id, encrypted_size);
    destination->is_ntt_form_= is_ntt_form;
    destination->scale_ = 1.0;

    // c[j] = public_key[j] * u + e[j] where e[j] <-- chi, u <-- R_3.

    // Generate u <-- R_3
    uint64_t* u = allocate_poly(coeff_count, coeff_mod_count);
    sample_poly_ternary(u, random, parms);

    // c[j] = u * public_key[j]
    for (size_t i = 0; i < coeff_mod_count; i++)
    {
        ntt_negacyclic_harvey(
            u + i * coeff_count,
            small_ntt_tables[i]);
        for (size_t j = 0; j < encrypted_size; j++)
        {
            dyadic_product_coeffsmallmod(
                u + i * coeff_count,
                pubkey_data->at(pubkey_data, j) + i * coeff_count,
                coeff_count,
                coeff_modulus->at(coeff_modulus, i),
                destination->at(destination, j) + i * coeff_count);

            // addition with e_0, e_1 is in non-NTT form.
            if (!is_ntt_form)
            {
                inverse_ntt_negacyclic_harvey(
                    destination->at(destination, j) + i * coeff_count,
                    small_ntt_tables[i]);
            }
        }
    }

    // Generate e_j <-- chi.
    // c[j] = public_key[j] * u + e[j]
    for (size_t j = 0; j < 2; j++)
    {
        sample_poly_normal(u, random, parms);
        for (size_t i = 0; i < coeff_mod_count; i++)
        {
            // addition with e_0, e_1 is in NTT form.
            if (is_ntt_form)
            {
                ntt_negacyclic_harvey(
                    u + i * coeff_count,
                    small_ntt_tables[i]);
            }
            add_poly_poly_coeffsmallmod(
                u + i * coeff_count,
                destination->at(destination, j) + i * coeff_count,
                coeff_count,
                coeff_modulus->at(coeff_modulus, i),
                destination->at(destination, j) + i * coeff_count);
        }
    }
    hedge_free(u);
}

void encrypt_zero_symmetric(
    const SecretKey* secret_key,
    hedge_context_t* context,
    parms_id_type* parms_id,
    struct rng* random,
    bool is_ntt_form,
    Ciphertext* destination)
{
    if (!EQUALS(secret_key->parms_id((SecretKey*)secret_key), &context->key_parms_id))
    {
        log_trace("encrypt_zero_symmetric(%p, %p,.)\n", secret_key, context);
        print_array_info(secret_key->sk_->parms_id_.data, 4);
        print_array_info(context->key_parms_id.data, 4);
        invalid_argument("key_parms_id mismatch");
    }
    ctxdata_t* context_data = context->get_ctxdata(context, parms_id);
    encrypt_parameters* parms = context_data->parms;
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus(parms);
    size_t coeff_mod_count = coeff_modulus->size;
    size_t coeff_count = parms->poly_modulus_degree(parms);
    SmallNTTTables** small_ntt_tables = context_data->small_ntt_tables;
    size_t encrypted_size = 2;

    destination->resize(destination, context, parms_id, encrypted_size);
    destination->is_ntt_form_ = is_ntt_form;
    destination->scale_ = 1.0;

    log_trace("destination->size_=%zu\n", destination->size_);
    // Generate ciphertext: (c[0], c[1]) = ([-(as+e)]_q, a)

    // Sample a uniformly at random
    // Set c[1] = a (we sample the NTT form directly)
    sample_poly_uniform(destination->at(destination, 1), random, parms);

    // Sample e <-- chi
    uint64_t* noise = allocate_poly(coeff_count, coeff_mod_count);
    sample_poly_normal(noise, random, parms);

    log_trace("** noise : coeff_count=%zu coeff_mod_count=%zu\n", coeff_count, coeff_mod_count);
    log_trace("sample_poly_normal(noise ...\n");
    //print_polynomial("noise", noise, coeff_count * coeff_mod_count, TRUE);
    // calculate -(a*s + e) (mod q) and store in c[0]
    for (size_t i = 0; i < coeff_mod_count; i++)
    {
        Plaintext* ptxt = secret_key->data((SecretKey*)secret_key);
        // Transform the noise e into NTT representation.
        ntt_negacyclic_harvey(
            noise + i * coeff_count,
            small_ntt_tables[i]);
        dyadic_product_coeffsmallmod(
            ptxt->data(ptxt) + i * coeff_count,
            destination->at(destination, 1) + i * coeff_count,
            coeff_count,
            coeff_modulus->at(coeff_modulus, i),
            destination->data(destination) + i * coeff_count);
        add_poly_poly_coeffsmallmod(
            noise + i * coeff_count,
            destination->data(destination) + i * coeff_count,
            coeff_count,
            coeff_modulus->at(coeff_modulus, i),
            destination->data(destination) + i * coeff_count);
        negate_poly_coeffsmallmod(
            destination->data(destination) + i * coeff_count,
            coeff_count,
            coeff_modulus->at(coeff_modulus, i),
            destination->data(destination) + i * coeff_count);
    }

    hedge_free(noise);

    log_trace("destination->size_=%zu\n", destination->size_);
    log_trace("destination->data_->count=%zu\n", destination->data_->count);
}