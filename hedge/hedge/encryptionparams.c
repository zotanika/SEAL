// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "encryptionparams.h"
#include "math/uintcore.h"
#include <limits.h>
#include "defines.h"
#include "lib/hash.h"
#include "lib/common.h"
#include "lib/vector.h"
#include "math/smallmodulus.h"

parms_id_type parms_id_zero = { { 0, 0, 0, 0 } }; //sha3_zero_block;

static void set_poly_modulus_degree(encrypt_parameters* self, size_t poly_modulus_degree)
{
    // Set the degree
    self->poly_modulus_degree_ = poly_modulus_degree;

    // Re-compute the parms_id
    self->compute_parms_id(self);
}

static void set_coeff_modulus(encrypt_parameters* self, 
            vector(Zmodulus)* coeff_modulus)
{
    // Set the coeff_modulus_
    if (coeff_modulus->size > HEDGE_COEFF_MOD_COUNT_MAX ||
        coeff_modulus->size < HEDGE_COEFF_MOD_COUNT_MIN)
    {
        invalid_argument("coeff_modulus is invalid");
    }

    self->coeff_modulus_->assign(self->coeff_modulus_, coeff_modulus);

    // Re-compute the parms_id
    self->compute_parms_id(self);
}

static void set_plain_modulus(encrypt_parameters* self, const Zmodulus* plain_modulus)
{
    // CKKS does not use plain_modulus
    if (self->scheme_ != BFV) {
        logic_error("unsupported scheme");
    }

    if (self->plain_modulus_)
        del_Zmodulus(self->plain_modulus_);
    self->plain_modulus_ = (Zmodulus *)plain_modulus;

    // Re-compute the parms_id
    self->compute_parms_id(self);
}

static void set_plain_modulus_uint64(encrypt_parameters* self, uint64_t plain_modulus)
{
    self->set_plain_modulus(self, new_Zmodulus(plain_modulus));
}

static scheme_type get_scheme(encrypt_parameters* self)
{
    return self->scheme_;
}

static size_t poly_modulus_degree(encrypt_parameters* self)
{
    return self->poly_modulus_degree_;
}

static vector(Zmodulus)* coeff_modulus(encrypt_parameters* self)
{
    return self->coeff_modulus_;
}

static Zmodulus* plain_modulus(encrypt_parameters* self)
{
    return self->plain_modulus_;
}

static void save(encrypt_parameters* self, bytestream_t* stream)
{
}

static void load(encrypt_parameters* self, bytestream_t* stream)
{
}

static bool is_valid_scheme(encrypt_parameters* self, uint8_t scheme)
{
    if (scheme == CKKS)
        return TRUE;
    
    return FALSE;
}

static parms_id_type* parms_id(encrypt_parameters* self)
{
    return &self->parms_id_;
}

static void compute_parms_id(encrypt_parameters* self)
{
    size_t idx;
    Zmodulus* mod;
    size_t coeff_mod_count = self->coeff_modulus_->size;

    size_t total_uint64_count = add_safe(
        1,  // scheme
        1,  // poly_modulus_degree
        coeff_mod_count,
        self->plain_modulus_->uint64_count
    );
    uint64_t* param_data = hedge_malloc(sizeof(uint64_t) * total_uint64_count);
    uint64_t *param_data_ptr = param_data;

    // Write the scheme identifier
    *param_data_ptr++ = (uint64_t)self->scheme_;

    // Write the poly_modulus_degree. Note that it will always be positive.
    *param_data_ptr++ = (uint64_t)(self->poly_modulus_degree_);
    iterate_vector(idx, mod, self->coeff_modulus_) {
        Zmodulus* sm = mod;
        *param_data_ptr++ = sm->value;
    }
    set_uint_uint(&self->plain_modulus_->value,
        self->plain_modulus_->uint64_count,
        param_data_ptr);
    param_data_ptr += self->plain_modulus_->uint64_count;
    sha3_hash(param_data, total_uint64_count, &self->parms_id_);
    // Did we somehow manage to get a zero block as result? This is reserved for
    // plaintexts to indicate non-NTT-transformed form.
    if (EQUALS(&self->parms_id_, &parms_id_zero))
    {
        logic_error("parms_id cannot be zero");
    }
    hedge_free(param_data);
}

encrypt_parameters* dup_encrypt_parameters(encrypt_parameters* src)
{
    encrypt_parameters* new = hedge_malloc(sizeof(encrypt_parameters));
    memcpy(new, src, sizeof(encrypt_parameters));
    new->coeff_modulus_ = dup_vector(Zmodulus, src->coeff_modulus_);
    new->plain_modulus_ = dup_Zmodulus(src->plain_modulus_);
    return new;
}

static void init(encrypt_parameters* self, uint8_t scheme)
{
    self->plain_modulus_ = new_Zmodulus(0);
    self->scheme_ = (scheme_type)scheme;
    self->compute_parms_id(self);
}

encrypt_parameters* new_encrypt_parameters(uint8_t scheme)
{
    encrypt_parameters* self = hedge_zalloc(sizeof(encrypt_parameters));
    self->coeff_modulus_ = new_vector(Zmodulus, 8);
    self->set_poly_modulus_degree = set_poly_modulus_degree;
    self->set_coeff_modulus = set_coeff_modulus;
    self->set_plain_modulus = set_plain_modulus;
    self->set_plain_modulus_uint64 = set_plain_modulus_uint64;
    self->scheme = get_scheme;
    self->poly_modulus_degree = poly_modulus_degree;
    self->coeff_modulus = coeff_modulus;
    self->plain_modulus = plain_modulus;
    self->save = save;
    self->load = load;
    self->is_valid_scheme = is_valid_scheme;
    self->parms_id = parms_id;
    self->compute_parms_id = compute_parms_id;
    init(self, scheme);
    return self;
}

void del_encrypt_parameters(encrypt_parameters* self)
{
    del_vector(self->coeff_modulus_);
    del_Zmodulus(self->plain_modulus_);
    //CHECK: this is currently freed in context.c del_Zmodulus(self->plain_modulus_);
    hedge_free(self);
}