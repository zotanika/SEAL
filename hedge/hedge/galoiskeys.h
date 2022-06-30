#ifndef __GALOISKEYS_H__
#define __GALOISKEYS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ciphertext.h"
#include "encryptionparams.h"
#include "kswitchkeys.h"
#include "lib/common.h"

typedef struct galois_keys GaloisKeys;
typedef struct galois_keys galois_keys_t;

/**
 * Class to store Galois keys.
 * 
 * @par Slot Rotations
 * Galois keys are used together with batching (BatchEncoder). If the polynomial modulus
 * is a polynomial of degree N, in batching the idea is to view a plaintext polynomial as
 * a 2-by-(N/2) matrix of integers modulo plaintext modulus. Normal homomorphic computations
 * operate on such encrypted matrices element (slot) wise. However, special rotation
 * operations allow us to also rotate the matrix rows cyclically in either direction, and
 * rotate the columns (swap the rows). These operations require the Galois keys.
 * 
 * @par Thread Safety
 * In general, reading from GaloisKeys is thread-safe as long as no other thread is
 * concurrently mutating it. This is due to the underlying data structure storing the
 * Galois keys not being thread-safe.
 * 
 * @see SecretKey for the class that stores the secret key.
 * @see PublicKey for the class that stores the public key.
 * @see RelinKeys for the class that stores the relinearization keys.
 * @see KeyGenerator for the class that generates the Galois keys.
 */
struct galois_keys {
    struct kswitch_keys *parent;
    size_t (*get_index)(uint64_t galois_elt);
    bool (*has_key)(struct galois_keys *self, uint64_t galois_elt);
    struct galois_keys *(*key)(struct galois_keys *self, uint64_t galois_elt);
};

extern struct galois_keys *new_galoiskeys(void);
extern void del_galoiskeys(struct galois_keys *self);

#ifdef __cplusplus
}
#endif

#endif /* __GALOISKEYS_H__ */