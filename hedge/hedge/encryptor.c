
#include "encryptor.h"
#include "math/smallmodulus.h"
#include "math/uintarith.h"
#include "math/polyarithsmallmod.h"
#include "math/smallntt.h"
#include "math/rlwe.h"
#include "math/rng.h"
#include "lib/debug.h"
#include "hedge_malloc.h"
#include "lib/common.h"
#include "lib/algorithm.h"
#include "math/baseconverter.h"
#include "valcheck.h"

static void init(Encryptor* self, hedge_context_t* context,
    const PublicKey* public_key)
{
    self->context_ = context;
    self->public_key_ = (PublicKey*)public_key;

    // Verify parameters
    if (!context)
    {
        invalid_argument("invalid context");
    }
    if (!context->is_parms_set(context))
    {
        invalid_argument("encryption parameters are not set correctly");
    }
    if (!EQUALS(public_key->parms_id((PublicKey*)public_key), &context->key_parms_id))
    {
        invalid_argument("public key is not valid for encryption parameters");
    }

    encrypt_parameters* parms = context->key_ctxdata(context)->parms;
	vector(Zmodulus)* coeff_modulus = parms->coeff_modulus_;
	size_t coeff_count = parms->poly_modulus_degree_;
	size_t coeff_mod_count = coeff_modulus->size;

    // Quick sanity check
    //if (!product_fits_in(coeff_count, coeff_mod_count, 2))
    //{
    //    logic_error("invalid parameters");
    //}
}

static void encrypt_zero(Encryptor* self, parms_id_type* parms_id,
    Ciphertext* destination)
{
    hedge_context_t* context = self->context_;
    // Verify parameters.
    ctxdata_t* context_data = context->get_ctxdata(context, parms_id);
    if (!context_data)
    {
        invalid_argument("parms_id is not valid for encryption parameters");
    }
    DEBUG_TRACE_LINE();
    encrypt_parameters* parms = context_data->parms;
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus_;
    DEBUG_TRACE_LINE();
	size_t coeff_count = parms->poly_modulus_degree_;
    DEBUG_TRACE_LINE();
	size_t coeff_mod_count = coeff_modulus->size;

    bool is_ntt_form = true;
    DEBUG_TRACE_LINE();
    // Resize destination and save results
    destination->resize(destination, self->context_, parms_id, 2);
    DEBUG_TRACE_LINE();
    ctxdata_t* prev_context_data_ptr = context_data->prev_context_data;
    ctxdata_t prev_context_data = *prev_context_data_ptr;
    DEBUG_TRACE_LINE();
    if (prev_context_data_ptr)
    {
        parms_id_type* prev_parms_id = &prev_context_data.parms->parms_id_;
        struct base_converter* base_converter = prev_context_data.base_converter;

        // Zero encryption without modulus switching
        DEBUG_TRACE_LINE();
        Ciphertext* temp = new_Ciphertext();
        DEBUG_TRACE_LINE();
        encrypt_zero_asymmetric(self->public_key_, self->context_, prev_parms_id,
            &self->random, is_ntt_form, temp);
        DEBUG_TRACE_LINE();
        if (temp->is_ntt_form(temp) != is_ntt_form)
        {
            invalid_argument("NTT form mismatch");
        }
        DEBUG_TRACE_LINE();
        // Modulus switching
        for (size_t j = 0; j < 2; j++)
        {
            if (is_ntt_form)
            {
                DEBUG_TRACE_LINE();
                base_converter->floor_last_coeff_modulus_ntt_inplace(base_converter,
                    temp->at(temp, j),
                    (const SmallNTTTables **)prev_context_data.small_ntt_tables);
            }
            else
            {
                DEBUG_TRACE_LINE();
                base_converter->floor_last_coeff_modulus_inplace(base_converter,
                    temp->at(temp, j));
            }
            DEBUG_TRACE_LINE();
            set_poly_poly_simple(
                temp->at(temp, j),
                coeff_count,
                coeff_mod_count,
                destination->at(destination, j));
            DEBUG_TRACE_LINE();
        }

        destination->is_ntt_form_ = is_ntt_form;

        // Need to set the scale here since encrypt_zero_asymmetric only sets
        // it for temp
        destination->scale_ = temp->scale_;
        del_Ciphertext(temp);
    }
    else
    {
        encrypt_zero_asymmetric(self->public_key_, self->context_,
            parms_id, &self->random, is_ntt_form, destination);
    }
}

static void encrypt(Encryptor* self, const Plaintext* plain, Ciphertext* destination)
{
    hedge_context_t* context = self->context_;
    DEBUG_TRACE_LINE();
    // Verify that plain is valid.
    if (!is_valid_for(Plaintext, plain, context))
    {
        invalid_argument("plain is not valid for encryption parameters");
    }

    if (!plain->is_ntt_form((Plaintext*)plain))
    {
        invalid_argument("plain must be in NTT form");
    }
    DEBUG_TRACE_LINE();
    ctxdata_t* context_data = context->get_ctxdata(context, (parms_id_type*)&plain->parms_id_);
    if (!context_data)
    {
        invalid_argument("plain is not valid for encryption parameters");
    }
    DEBUG_TRACE_LINE();
    encrypt_parameters* parms = context_data->parms;
    DEBUG_TRACE_LINE();
	vector(Zmodulus)* coeff_modulus = parms->coeff_modulus_;
    DEBUG_TRACE_LINE();
	size_t coeff_count = parms->poly_modulus_degree_;
	size_t coeff_mod_count = coeff_modulus->size;
    DEBUG_TRACE_LINE();
    encrypt_zero(self, &context_data->parms->parms_id_, destination);
    DEBUG_TRACE_LINE();
    // The plaintext gets added into the c_0 term of ciphertext (c_0,c_1).
    for (size_t i = 0; i < coeff_mod_count; i++)
    {
        add_poly_poly_coeffsmallmod(
            destination->data_->data + (i * coeff_count),
            plain->data_->data + (i * coeff_count),
            coeff_count,
            coeff_modulus->at(coeff_modulus, i),
            destination->data_->data + (i * coeff_count));
    }
    DEBUG_TRACE_LINE();
    destination->scale_ = plain->scale_;
    DEBUG_TRACE_LINE();
}

void del_Encryptor(Encryptor* self)
{
    hedge_free(self);
}

Encryptor* new_Encryptor(hedge_context_t* context, PublicKey* pubkey)
{
    Encryptor* self = hedge_zalloc(sizeof(Encryptor));

    self->encrypt_zero = encrypt_zero;
    self->encrypt = encrypt;

    init(self, context, pubkey);

    return self;
}
