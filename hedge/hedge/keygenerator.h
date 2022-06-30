#ifndef __KEYGENERATOR_H__
#define __KEYGENERATOR_H__

#include "context.h"
#include "publickey.h"
#include "secretkey.h"
#include "relinkeys.h"
#include "galoiskeys.h"
#include "math/smallntt.h"
#include "lib/vector.h"

typedef struct key_generator key_generator_t;
typedef struct key_generator KeyGenerator;

/**
 * Generates matching secret key and public key. An existing KeyGenerator can
 * also at any time be used to generate relinearization keys and Galois keys.
 * Constructing a KeyGenerator requires only a SEALContext.
 * 
 * @see EncryptionParameters for more details on encryption parameters.
 * @see SecretKey for more details on secret key.
 * @see PublicKey for more details on public key.
 * @see RelinKeys for more details on relinearization keys.
 * @see GaloisKeys for more details on Galois keys.
 */
struct key_generator {
    PublicKey *public_key;
    SecretKey *secret_key;
    struct hedge_context *context;

    uint64_t *secret_key_array;
    size_t secret_key_array_size;
    bool sk_generated;
    bool pk_generated;

    RelinKeys *generated_relinkeys;
    
    /**
     * Creates a key_generator initialized with the specified hedge_context.
     * 
     * @param[in] hedge_context
     * @throws std::invalid_argument if the context is not set or encryption
     * parameters are not valid
     */
    void (*init)(struct key_generator *self, struct hedge_context *ctx);
    /**
     * Creates a key_generator instance initialized with the specified hedge_context
     * and specified previously secret key. This can e.g. be used to increase
     * the number of relinearization keys from what had earlier been generated,
     * or to generate Galois keys in case they had not been generated earlier.
     * 
     * @param[in] hedge_context
     * @param[in] skey A previously generated secret key
     * @throws std::invalid_argument if encryption parameters are not valid
     * @throws std::invalid_argument if secret_key or public_key is not valid
     * for encryption parameters
     */
    void (*init_with_skey)(struct key_generator *self, struct hedge_context *ctx,
                const SecretKey *skey);
    /**
     * Creates an KeyGenerator instance initialized with the specified SEALContext
     * and specified previously secret and public keys. This can e.g. be used
     * to increase the number of relinearization keys from what had earlier been
     * generated, or to generate Galois keys in case they had not been generated
     * earlier.
     * 
     * @param[in] hedge_context
     * @param[in] skey A previously generated secret key
     * @param[in] pkey A previously generated public key
     * @throws std::invalid_argument if encryption parameters are not valid
     * @throws std::invalid_argument if secret_key or public_key is not valid
     * for encryption parameters
     */
    void (*init_with_skeynpkey)(struct key_generator *self, struct hedge_context *ctx,
                const SecretKey *skey, const PublicKey *pkey);
    void (*free)(struct key_generator *self);
    SecretKey *(*get_secret_key)(struct key_generator *self);
    PublicKey *(*get_public_key)(struct key_generator *self);
    void (*generate_secretkey)(struct key_generator *self, bool is_initialized);
    void (*generate_publickey)(struct key_generator *self);
    void (*generate_kswitchkeys)(struct key_generator *self, const uint64_t *new_keys,
                size_t num_keys, struct kswitch_keys *dest);
    void (*generate_one_kswitchkey)(struct key_generator *self, const uint64_t *new_key,
                vector_publickey *dest);
    void (*compute_secret_key_array)(struct key_generator *self,
                struct ctxdata *context_data, size_t max_power);
    /**
     * Generates and returns the specified number of relinearization keys.
     * 
     * @param[in] count The number of relinearization keys to generate
     * @throws std::invalid_argument if count is zero or too large
     */
    relinkeys_t *(*relin_keys)(struct key_generator *self, size_t count);
    /**
     * Generates and returns Galois keys. This function creates specific Galois
     * keys that can be used to apply specific Galois automorphisms on encrypted
     * data. The user needs to give as input a vector of Galois elements
     * corresponding to the keys that are to be created.
     * 
     * The Galois elements are odd integers in the interval [1, M-1], where
     * M = 2*N, and N = degree(poly_modulus). Used with batching, a Galois element
     * 3^i % M corresponds to a cyclic row rotation i steps to the left, and
     * a Galois element 3^(N/2-i) % M corresponds to a cyclic row rotation i
     * steps to the right. The Galois element M-1 corresponds to a column rotation
     * (row swap) in BFV, and complex conjugation in CKKS. In the polynomial view
     * (not batching), a Galois automorphism by a Galois element p changes Enc(plain(x))
     * to Enc(plain(x^p)).
     * 
     *  @param[in] galois_elts The Galois elements for which to generate keys
     *  @throws std::invalid_argument if the Galois elements are not valid
     */
    galois_keys_t *(*galois_keys_by_elts)(struct key_generator *self,
                const vector(uint64_t) *galois_elts);
    /**
     * Generates and returns Galois keys. This function creates specific Galois
     * keys that can be used to apply specific Galois automorphisms on encrypted
     * data. The user needs to give as input a vector of desired Galois rotation
     * step counts, where negative step counts correspond to rotations to the
     * right and positive step counts correspond to rotations to the left.
     * A step count of zero can be used to indicate a column rotation in the BFV
     * scheme complex conjugation in the CKKS scheme.
     * 
     * @param[in] galois_elts The rotation step counts for which to generate keys
     * @throws std::logic_error if the encryption parameters do not support batching
     * and scheme is scheme_type::BFV
     * @throws std::invalid_argument if the step counts are not valid
     */
    galois_keys_t *(*galois_keys_by_step)(struct key_generator *self,
                const vector(int) *steps);
    /**
     * Generates and returns Galois keys. This function creates logarithmically
     * many (in degree of the polynomial modulus) Galois keys that is sufficient
     * to apply any Galois automorphism (e.g. rotations) on encrypted data. Most
     * users will want to use this overload of the function.
     */
    galois_keys_t *(*galois_keys)(struct key_generator *self);
};

extern struct key_generator *new_keygenerator(struct hedge_context *context);
extern void del_keygenerator(struct key_generator *self);

#endif /* __KEYGENERATOR_H__ */