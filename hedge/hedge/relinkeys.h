#ifndef __RELINKEYS_H__
#define __RELINKEYS_H__

#include "ciphertext.h"
#include "encryptionparams.h"
#include "kswitchkeys.h"
#include "lib/types.h"

typedef struct relinkeys RelinKeys;
typedef struct relinkeys relinkeys_t;

/**
 * Class to store relinearization keys.
 * 
 * @par Relinearization
 * Freshly encrypted ciphertexts have a size of 2, and multiplying ciphertexts
 * of sizes K and L results in a ciphertext of size K+L-1. Unfortunately, this
 * growth in size slows down further multiplications and increases noise growth.
 * Relinearization is an operation that has no semantic meaning, but it reduces
 * the size of ciphertexts back to 2. Microsoft SEAL can only relinearize size 3
 * ciphertexts back to size 2, so if the ciphertexts grow larger than size 3,
 * there is no way to reduce their size. Relinearization requires an instance of
 * RelinKeys to be created by the secret key owner and to be shared with the
 * evaluator. Note that plain multiplication is fundamentally different from
 * normal multiplication and does not result in ciphertext size growth.
 * 
 * @par When to Relinearize
 * Typically, one should always relinearize after each multiplications. However,
 * in some cases relinearization should be postponed as late as possible due to
 * its computational cost. For example, suppose the computation involves several
 * homomorphic multiplications followed by a sum of the results. In this case it
 * makes sense to not relinearize each product, but instead add them first and
 * only then relinearize the sum. This is particularly important when using the
 * CKKS scheme, where relinearization is much more computationally costly than
 * multiplications and additions.
 * 
 * @par Thread Safety
 * In general, reading from RelinKeys is thread-safe as long as no other thread
 * is concurrently mutating it. This is due to the underlying data structure
 * storing the relinearization keys not being thread-safe.
 * 
 * @see SecretKey for the class that stores the secret key.
 * @see PublicKey for the class that stores the public key.
 * @see GaloisKeys for the class that stores the Galois keys.
 * @see KeyGenerator for the class that generates the relinearization keys.
 */
struct relinkeys {
    struct kswitch_keys *parent;
    size_t (*get_index)(relinkeys_t *self, size_t key_power);
    bool (*has_key)(relinkeys_t *self, size_t key_power);
    relinkeys_t *(*key)(relinkeys_t *self, size_t key_power);
};

extern relinkeys_t *new_relinkeys(void);
extern void del_relinkeys(relinkeys_t *self);

#endif /* __RELINKEYS_H__ */