#ifndef __VALCHECK_H__
#define __VALCHECK_H__

#include "lib/types.h"
#include "context.h"
#include "plaintext.h"
#include "ciphertext.h"
#include "secretkey.h"
#include "publickey.h"
#include "kswitchkeys.h"
#include "relinkeys.h"
#include "galoiskeys.h"

/**
 * Check whether the given major data structure holds valid for a given
 * hedge_context. If the given hedge_context is not set, the encryption
 * parameters are invalid, or the target data does not match the hedge_context,
 * this function returns false. Otherwise, returns true.
 * This function only checks the metadata and not the target data itself.
 * 
 * @param[_T] may hold Plaintext, Ciphertext, SecretKey, PublicKey, KSwitchKeys,
 *            RelinKeys, GaloisKeys
 * @param[_in] pointer of Plaintext, Ciphertext, SecretKey, PublicKey, KSwitchKeys,
 *            RelinKeys, GaloisKeys
 * @param[_ctxt] pointer of hedge_context
 */
#define is_metadata_valid_for(_T, _in, _ctxt) \
        is_##_T##_metadata_valid_for((const _T *)_in, (const struct hedge_context *)_ctxt)

/**
 * Check whether the given major data structure holds valid for a given
 * hedge_context. If the given hedge_context is not set, the encryption
 * parameters are invalid, or the target data does not match the hedge_context,
 * this function returns false. Otherwise, returns true.
 * 
 * @param[_T] may hold Plaintext, Ciphertext, SecretKey, PublicKey, KSwitchKeys,
 *            RelinKeys, GaloisKeys
 * @param[_in] pointer of Plaintext, Ciphertext, SecretKey, PublicKey, KSwitchKeys,
 *            RelinKeys, GaloisKeys
 * @param[_ctxt] pointer of hedge_context
 */
#define is_valid_for(_T, _in, _ctxt) \
        is_##_T##_valid_for((const _T *)_in, (const struct hedge_context *)_ctxt)

extern bool is_Plaintext_metadata_valid_for(
        const Plaintext *in,
        const struct hedge_context *context);

extern bool is_Ciphertext_metadata_valid_for(
        const Ciphertext *in,
        const struct hedge_context *context);

extern bool is_SecretKey_metadata_valid_for(
        const SecretKey *in,
        const struct hedge_context *context);

extern bool is_PublicKey_metadata_valid_for(
        const PublicKey *in,
        const struct hedge_context *context);

extern bool is_KSwitchKeys_metadata_valid_for(
        const KSwitchKeys *in,
        const struct hedge_context *context);

extern bool is_RelinKeys_metadata_valid_for(
        const RelinKeys *in,
        const struct hedge_context *context);

extern bool is_GaloisKeys_metadata_valid_for(
        const GaloisKeys *in,
        const struct hedge_context *context);

extern bool is_Plaintext_valid_for(
        const Plaintext *in,
        const struct hedge_context *context);

extern bool is_Ciphertext_valid_for(
        const Ciphertext *in,
        const struct hedge_context *context);

extern bool is_SecretKey_valid_for(
        const SecretKey *in,
        const struct hedge_context *context);

extern bool is_PublicKey_valid_for(
        const PublicKey *in,
        const struct hedge_context *context);

extern bool is_KSwitchKeys_valid_for(
        const KSwitchKeys *in,
        const struct hedge_context *context);

extern bool is_RelinKeys_valid_for(
        const RelinKeys *in,
        const struct hedge_context *context);

extern bool is_GaloisKeys_valid_for(
        const GaloisKeys *in,
        const struct hedge_context *context);

#endif /* __VALCHECK_H__ */