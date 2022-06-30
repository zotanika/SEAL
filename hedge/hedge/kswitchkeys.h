#ifndef __KSWITCHKEYS_H__
#define __KSWITCHKEYS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lib/vector.h"
#include "lib/bytestream.h"
#include "publickey.h"
#include "encryptionparams.h"

typedef struct kswitch_keys KSwitchKeys;
typedef struct kswitch_keys kswitch_keys_t;

DECLARE_VECTOR_TYPE(PublicKey);
typedef vector(PublicKey) vector_publickey;

DECLARE_VECTOR_TYPE(vector_publickey);

/**
 * Class to store keyswitching keys. It should never be necessary for normal
 * users to create an instance of KSwitchKeys. This class is used strictly as
 * a base class for RelinKeys and GaloisKeys classes.
 * 
 * @par Keyswitching
 * Concretely, keyswitching is used to change a ciphertext encrypted with one
 * key to be encrypted with another key. It is a general technique and is used
 * in relinearization and Galois rotations. A keyswitching key contains a sequence
 * (vector) of keys. In RelinKeys, each key is an encryption of a power of the
 * secret key. In GaloisKeys, each key corresponds to a type of rotation.
 * 
 * @par Thread Safety
 * In general, reading from KSwitchKeys is thread-safe as long as no
 * other thread is concurrently mutating it. This is due to the underlying
 * data structure storing the keyswitching keys not being thread-safe.
 * 
 * @see RelinKeys for the class that stores the relinearization keys.
 * @see GaloisKeys for the class that stores the Galois keys.
 */
struct kswitch_keys {
    parms_id_type *parms_id;
    vector(vector_publickey) *keys;

    void (*init)(struct kswitch_keys *self);
    void (*assign)(struct kswitch_keys *self, struct kswitch_keys *src);
    void (*free)(struct kswitch_keys *self);
    size_t (*size)(struct kswitch_keys *self);
    vector_publickey *(*data)(struct kswitch_keys *self, size_t index);
    void (*save)(struct kswitch_keys *self, const bytestream_t *stream);
    void (*unsafe_load)(struct kswitch_keys *self, bytestream_t *stream);
    void (*load)(struct kswitch_keys *self, struct hedge_context *context, bytestream_t *stream);
};

extern struct kswitch_keys *new_kswitchkeys(void);
extern void del_kswitchkeys(struct kswitch_keys *self);

#ifdef __cplusplus
}
#endif

#endif /* __KSWITCHKEYS_H__ */