#ifndef __RLWE_H__
#define __RLWE_H__

#include "publickey.h"
#include "secretkey.h"
#include "context.h"
#include "plaintext.h"
#include "ciphertext.h"

void sample_poly_ternary(
    uint64_t *poly,
    struct rng* random,
    const encrypt_parameters* parms);

void sample_poly_normal(
    uint64_t *poly,
    struct rng* random,
    const encrypt_parameters* parms);

void sample_poly_uniform(
    uint64_t *poly,
    struct rng* random,
    const encrypt_parameters* parms);

void encrypt_zero_asymmetric(
    const PublicKey* public_key,
    hedge_context_t* context,
    parms_id_type* parms_id,
    struct rng* random,
    bool is_ntt_form,
    Ciphertext* destination);

void encrypt_zero_symmetric(
    const SecretKey* secret_key,
    hedge_context_t* context,
    parms_id_type* parms_id,
    struct rng* random,
    bool is_ntt_form,
    Ciphertext* destination);

#endif /* __RLWE_H__ */