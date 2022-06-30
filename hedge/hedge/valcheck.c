#include "defines.h"
#include "valcheck.h"
#include "lib/common.h"

bool is_Plaintext_metadata_valid_for(const Plaintext *in, const struct hedge_context *context)
{
    struct encrypt_parameters *parms;
    vector(Zmodulus) *coeff_modulus;
    size_t pmd;

    if (!context || !context->is_parms_set((struct hedge_context *)context))
        return false;

    if (in->is_ntt_form((Plaintext *)in)) {
        /* Are the parameters valid for given plaintext? This check is slightly
         * non-trivial because we need to consider both the case where key_parms_id
         * equals first_parms_id, and cases where they are different. */
        struct ctxdata *context_data_ptr
                = (struct ctxdata *)context->get_ctxdata((struct hedge_context *)context, in->parms_id((Plaintext *)in));

        if (!context_data_ptr ||
            context_data_ptr->chain_index > context->first_ctxdata((struct hedge_context *)context)->chain_index)
            return false;

        parms = context_data_ptr->parms;
        coeff_modulus = parms->coeff_modulus(parms);
        pmd = parms->poly_modulus_degree(parms);
        if (mul_safe(coeff_modulus->size, pmd) != in->coeff_count((Plaintext *)in))
            return false;
    } else {
        parms = ((struct ctxdata *)context->first_ctxdata)->parms;
        if (parms->scheme(parms) != BFV)
            return false;

        pmd = parms->poly_modulus_degree(parms);
        if (in->coeff_count((Plaintext *)in) > pmd)
            return false;
    }
    return true;
}

bool is_Ciphertext_metadata_valid_for(const Ciphertext *in, const struct hedge_context *context)
{
    struct ctxdata *context_data_ptr;
    struct encrypt_parameters *parms;
    vector(Zmodulus) *coeff_modulus;
    size_t pmd;

    if (!context || !context->is_parms_set((struct hedge_context *)context))
        return false;

    /* Are the parameters valid for given ciphertext? This check is slightly
     * non-trivial because we need to consider both the case where key_parms_id
     * equals first_parms_id, and cases where they are different. */
    context_data_ptr = context->get_ctxdata((struct hedge_context *)context, in->parms_id((Ciphertext *)in));
    if (!context_data_ptr ||
        context_data_ptr->chain_index > context->first_ctxdata((struct hedge_context *)context)->chain_index)
        return false;

    /* Check that the metadata matches */
    coeff_modulus = context_data_ptr->parms->coeff_modulus(context_data_ptr->parms);
    pmd = context_data_ptr->parms->poly_modulus_degree(context_data_ptr->parms);
    if ((coeff_modulus->size != in->coeff_mod_count((Ciphertext *)in)) ||
        (pmd != in->poly_modulus_degree_))
        return false;

    /* Check that size is either 0 or within right bounds */
    if ((in->size((Ciphertext *)in) < HEDGE_CIPHERTEXT_SIZE_MIN && in->size((Ciphertext *)in) != 0) ||
        in->size((Ciphertext *)in) > HEDGE_CIPHERTEXT_SIZE_MAX)
        return false;

    return true;
}

bool is_SecretKey_metadata_valid_for(const SecretKey *in, const struct hedge_context *context)
{
    struct ctxdata *context_data_ptr;
    struct encrypt_parameters *parms;
    vector(Zmodulus) *coeff_modulus;
    Plaintext *inside;
    size_t pmd;

    if (!context || !context->is_parms_set((struct hedge_context *)context))
        return false;

    if (in->parms_id((SecretKey *)in) != &context->key_parms_id)
        return false;

    context_data_ptr = context->key_ctxdata((struct hedge_context *)context);
    parms = context_data_ptr->parms;
    coeff_modulus = parms->coeff_modulus(parms);
    pmd = parms->poly_modulus_degree(parms);
    inside = in->data((SecretKey *)in);
    if (mul_safe(coeff_modulus->size, pmd) != inside->coeff_count(inside))
        return false;

    return true;
}

bool is_PublicKey_metadata_valid_for(const PublicKey *in, const struct hedge_context *context)
{
    struct ctxdata *context_data_ptr;
    struct encrypt_parameters *parms;
    vector(Zmodulus) *coeff_modulus;
    Ciphertext *inside;
    size_t pmd;

    if (!context || !context->is_parms_set((struct hedge_context *)context))
        return false;

    if (in->parms_id((PublicKey *)in) != &context->key_parms_id ||
        !in->data((PublicKey *)in)->is_ntt_form_)
        return false;

    context_data_ptr = context->key_ctxdata((struct hedge_context *)context);
    parms = context_data_ptr->parms;
    coeff_modulus = parms->coeff_modulus(parms);
    pmd = parms->poly_modulus_degree(parms);
    inside = in->data((PublicKey *)in);
    if ((coeff_modulus->size != inside->coeff_mod_count(inside)) ||
        (pmd != inside->poly_modulus_degree_))
        return false;

    if (inside->size(inside) != HEDGE_CIPHERTEXT_SIZE_MIN)
        return false;

    return true;
}

bool is_KSwitchKeys_metadata_valid_for(const KSwitchKeys *in, const struct hedge_context *context)
{
    size_t i, j;
    vector_publickey *inside;

    if (!context || !context->is_parms_set((struct hedge_context *)context))
        return false;

    if (in->parms_id != &context->key_parms_id)
        return false;

    for (i = 0; i < in->size((KSwitchKeys *)in); i++) {
        inside = in->data((KSwitchKeys *)in, i);
        for (j = 0; j < inside->size; j++) {
            if (!is_metadata_valid_for(PublicKey, (const PublicKey *)inside->at(inside, j), context))
                return false;
        }
    }
    return true;
}

bool is_RelinKeys_metadata_valid_for(const RelinKeys *in, const struct hedge_context *context)
{
    size_t size = in->parent->size(in->parent);

    if (!(size <= HEDGE_CIPHERTEXT_SIZE_MAX - 2 && size >= HEDGE_CIPHERTEXT_SIZE_MIN - 2))
        return false;

    return is_metadata_valid_for(KSwitchKeys,
                    (const KSwitchKeys *)(container_of(in, RelinKeys, parent)), context);
}

bool is_GaloisKeys_metadata_valid_for(const GaloisKeys *in, const struct hedge_context *context)
{
    return is_metadata_valid_for(KSwitchKeys,
                    (const KSwitchKeys *)(container_of(in, GaloisKeys, parent)), context);
}

bool is_Plaintext_valid_for(const Plaintext *in, const struct hedge_context *context)
{
    size_t i;

    if (!is_metadata_valid_for(Plaintext, in, context))
        return false;

    if (in->is_ntt_form((Plaintext *)in)) {
        struct ctxdata *context_data_ptr =
            context->get_ctxdata((struct hedge_context *)context,
            in->parms_id((Plaintext *)in));
        struct encrypt_parameters *parms = context_data_ptr->parms;
        vector(Zmodulus) *coeff_modulus = parms->coeff_modulus(parms);
        size_t coeff_mod_count = coeff_modulus->size;
        uint64_t *ptr = in->data((Plaintext *)in);

        for (i = 0; i < coeff_mod_count; i++) {
            uint64_t modulus = coeff_modulus->at(coeff_modulus, i)->value;
            log_trace("@@@@ modulus=%lx\n", modulus);
            size_t poly_modulus_degree = parms->poly_modulus_degree(parms);
            for (; poly_modulus_degree--; ptr++) {
                if (poly_modulus_degree <= 4) {
                    log_trace(">>>>> poly_modulus_degree=%lu, *ptr=%lx modulus=%lx\n",
                        poly_modulus_degree, *ptr, modulus);
                }
                if (*ptr >= modulus) {
                    log_fatal("is_Plaintext_valid_for: invalid coeffecient at %zu : coefficient is greater than the modulus %lx >= %lx\n",
                        poly_modulus_degree, *ptr, modulus);
                    return false;
                }
            }
        }
    } else {
        struct encrypt_parameters *parms = context->first_ctxdata((struct hedge_context *)context)->parms;
        Zmodulus *modulus = parms->plain_modulus(parms);
        uint64_t modulo = modulus->value;
        uint64_t *ptr = in->data((Plaintext *)in);
        size_t size = in->coeff_count((Plaintext *)in);

        for (i = 0; i < size; i++, ptr++) {
            if (*ptr >= modulo)
                return false;
        }
    }
    return true;
}

bool is_Ciphertext_valid_for(const Ciphertext *in, const struct hedge_context *context)
{
    struct ctxdata *context_data_ptr;
    vector(Zmodulus) *coeff_modulus;
    size_t i, j;
    uint64_t *ptr = in->data((Ciphertext *)in);

    if (!is_metadata_valid_for(Ciphertext, in, context))
        return false;

    context_data_ptr = context->get_ctxdata((struct hedge_context *)context, in->parms_id((Ciphertext *)in));
    coeff_modulus = context_data_ptr->parms->coeff_modulus(context_data_ptr->parms);

    for (i = 0; i < in->size((Ciphertext *)in); i++) {
        for (j = 0; j < coeff_modulus->size; j++) {
            Zmodulus *this = coeff_modulus->at(coeff_modulus, j);
            uint64_t modulo = this->value;
            uint64_t poly_modulus_degree = in->poly_modulus_degree_;
            for (; poly_modulus_degree--; ptr++) {
                if (*ptr >= modulo)
                    return false;
            }
        }
    }
    return true;
}

bool is_SecretKey_valid_for(const SecretKey *in, const struct hedge_context *context)
{
    struct ctxdata *context_data_ptr;
    vector(Zmodulus) *coeff_modulus;
    size_t i;
    Plaintext *indata = in->data((SecretKey *)in);
    uint64_t *ptr = indata->data(indata);

    if (!is_metadata_valid_for(SecretKey, in, context))
        return false;

    context_data_ptr = context->key_ctxdata((struct hedge_context *)context);
    coeff_modulus = context_data_ptr->parms->coeff_modulus(context_data_ptr->parms);

    for (i = 0; i < coeff_modulus->size; i++) {
        Zmodulus *this = coeff_modulus->at(coeff_modulus, i);
        uint64_t modulo = this->value;
        size_t poly_modulus_degree = context_data_ptr->parms->poly_modulus_degree_;
        for (; poly_modulus_degree--; ptr++) {
            if (*ptr >= modulo)
                return false;
        }
    }
    return true;
}

bool is_PublicKey_valid_for(const PublicKey *in, const struct hedge_context *context)
{
    struct ctxdata *context_data_ptr;
    vector(Zmodulus) *coeff_modulus;
    size_t i, j;
    Ciphertext *indata = in->data((PublicKey *)in);
    uint64_t *ptr = indata->data(indata);

    if (!is_metadata_valid_for(PublicKey, in, context))
        return false;

    context_data_ptr = context->key_ctxdata((struct hedge_context *)context);
    coeff_modulus = context_data_ptr->parms->coeff_modulus(context_data_ptr->parms);

    for (i = 0; i < indata->size(indata); i++) {
        for (j = 0; j < coeff_modulus->size; j++) {
            Zmodulus *this = coeff_modulus->at(coeff_modulus, j);
            uint64_t modulo = this->value;
            size_t poly_modulus_degree = indata->poly_modulus_degree_;
            for (; poly_modulus_degree--; ptr++) {
                if (*ptr >= modulo)
                    return false;
            }
        }
    }
    return true;
}

bool is_KSwitchKeys_valid_for(const KSwitchKeys *in, const struct hedge_context *context)
{
    size_t i, j;
    vector_publickey *inside;

    if (!context || !context->is_parms_set((struct hedge_context *)context))
        return false;

    if (in->parms_id != &context->key_parms_id)
        return false;

    for (i = 0; i < in->size((KSwitchKeys *)in); i++) {
        inside = in->data((KSwitchKeys *)in, i);
        for (j = 0; j < inside->size; j++) {
            if (!is_valid_for(PublicKey, (const PublicKey *)inside->at(inside, j), context))
                return false;
        }
    }
    return true;
}

bool is_RelinKeys_valid_for(const RelinKeys *in, const struct hedge_context *context)
{
    return is_valid_for(KSwitchKeys,
                    (const KSwitchKeys *)container_of(in, RelinKeys, parent), context);
}

bool is_GaloisKeys_valid_for(const GaloisKeys *in, const struct hedge_context *context)
{
    return is_valid_for(KSwitchKeys,
                    (const KSwitchKeys *)container_of(in, GaloisKeys, parent), context);
}