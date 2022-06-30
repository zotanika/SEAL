#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "encryptionparams.h"
#include "lib/unordered_map.h"
#include "hedge_malloc.h"
#include "lib/list.h"
#include "math/modulus.h"
#include "math/smallntt.h"

typedef struct encrypt_parm_qualifiers encrypt_parm_qualifiers_t;
typedef struct hedge_context hedge_context_t;
typedef struct hedge_context HedgeContext;
typedef struct ctxdata ctxdata_t;
typedef struct ctxdata * ctxdata_ptr;

/**
 * Encryption Parameter Qualifiers
 * 
 * Stores a set of attributes (qualifiers) of a set of encryption parameters.
 * These parameters are mainly used internally in various parts of the library,
 * e.g., to determine which algorithmic optimizations the current support. The
 * qualifiers are automatically created by hedge_context class, silently passed
 * on to classes such as Encryptor, Evaluator, and Decryptor, and the only way to
 * change them is by changing the encryption parameters themselves. In other
 * words, a user will never have to create their own instance of this class, and
 * in most cases never have to worry about it at all.
 */
struct encrypt_parm_qualifiers {
    /**
     * If the encryption parameters are set in a way that is considered valid by
     * Microsoft SEAL, the variable parameters_set is set to true.
     */
    bool parameters_set;
    /**
     * Tells whether FFT can be used for polynomial multiplication. If the
     * polynomial modulus is of the form X^N+1, where N is a power of two, then
     * FFT can be used for fast multiplication of polynomials modulo the polynomial
     * modulus. In this case the variable using_fft will be set to true. However,
     * currently Microsoft SEAL requires this to be the case for the parameters
     * to be valid. Therefore, parameters_set can only be true if using_fft is
     * true.
     */
    bool using_fft;
    /**
     * Tells whether NTT can be used for polynomial multiplication. If the primes
     * in the coefficient modulus are congruent to 1 modulo 2N, where X^N+1 is the
     * polynomial modulus and N is a power of two, then the number-theoretic
     * transform (NTT) can be used for fast multiplications of polynomials modulo
     * the polynomial modulus and coefficient modulus. In this case the variable
     * using_ntt will be set to true. However, currently Microsoft SEAL requires
     * this to be the case for the parameters to be valid. Therefore, parameters_set
     * can only be true if using_ntt is true.
     */
    bool using_ntt;
    /**
     * Tells whether batching is supported by the encryption parameters. If the
     * plaintext modulus is congruent to 1 modulo 2N, where X^N+1 is the polynomial
     * modulus and N is a power of two, then it is possible to use the BatchEncoder
     * class to view plaintext elements as 2-by-(N/2) matrices of integers modulo
     * the plaintext modulus. This is called batching, and allows the user to
     * operate on the matrix elements (slots) in a SIMD fashion, and rotate the
     * matrix rows and columns. When the computation is easily vectorizable, using
     * batching can yield a huge performance boost. If the encryption parameters
     * support batching, the variable using_batching is set to true.
     */
    bool using_batching;
    /**
     * Tells whether fast plain lift is supported by the encryption parameters.
     * A certain performance optimization in multiplication of a ciphertext by
     * a plaintext (Evaluator::multiply_plain) and in transforming a plaintext
     * element to NTT domain (Evaluator::transform_to_ntt) can be used when the
     * plaintext modulus is smaller than each prime in the coefficient modulus.
     * In this case the variable using_fast_plain_lift is set to true.
     */
    bool using_fast_plain_lift;
    /**
     * Tells whether the coefficient modulus consists of a set of primes that
     * are in decreasing order. If this is true, certain modular reductions in
     * base conversion can be omitted, improving performance.
     */
    bool using_descending_modulus_chain;
    /**
     * Tells whether the encryption parameters are secure based on the standard
     * parameters from HomomorphicEncryption.org security standard.
     */
    sec_level_type sec_level;

    void (*init)(struct encrypt_parm_qualifiers *self);
    void (*free)(struct encrypt_parm_qualifiers *self);
};

extern struct encrypt_parm_qualifiers *new_epq(void);
extern void del_epq(struct encrypt_parm_qualifiers *self);

/**
 * Class to hold pre-computation data for a given set of encryption parameters.
 */
struct ctxdata {
    struct encrypt_parameters *parms;
    struct encrypt_parm_qualifiers *qualifiers;
    struct base_converter *base_converter;
    SmallNTTTables **small_ntt_tables;
    SmallNTTTables **plain_ntt_tables;

    uint64_t *total_coeff_modulus;
    uint64_t *coeff_div_plain_modulus;
    uint64_t *plain_upper_half_increment;
    uint64_t *upper_half_threshold;
    uint64_t *upper_half_increment;

    int total_coeff_modulus_bit_count;
    uint64_t plain_upper_half_threshold;

    struct ctxdata *prev_context_data;
    struct ctxdata *next_context_data;

    size_t chain_index;

    void (*init)(struct ctxdata *self, const struct encrypt_parameters *parms);
    void (*copy)(struct ctxdata *self, struct ctxdata *src);
    void (*move)(struct ctxdata *self, struct ctxdata *src);
    void (*free)(struct ctxdata *self);
};

extern struct ctxdata *new_ctxdata(const struct encrypt_parameters *parms);
extern void del_ctxdata(struct ctxdata *self);

DECLARE_UNORDERED_MAP_TYPE(parms_id_type, ctxdata_ptr);

/**
 * Performs sanity checks (validation) and pre-computations for a given set of encryption
 * parameters. While the encrypt_parameters class is intended to be a light-weight class
 * to store the encryption parameters, the hedge_context_t class is a heavy-weight class that
 * is constructed from a given set of encryption parameters. It validates the parameters
 * for correctness, evaluates their properties, and performs and stores the results of
 * several costly pre-computations.
 * 
 * After the user has set at least the poly_modulus, coeff_modulus, and plain_modulus
 * parameters in a given encrypt_parameters instance, the parameters can be validated
 * for correctness and functionality by constructing an instance of hedge_context_t. The
 * constructor of hedge_context_t does all of its work automatically, and concludes by
 * constructing and storing an instance of the EncryptionParameterQualifiers class, with
 * its flags set according to the properties of the given parameters. If the created
 * instance of EncryptionParameterQualifiers has the parameters_set flag set to true, the
 * given parameter set has been deemed valid and is ready to be used. If the parameters
 * were for some reason not appropriately set, the parameters_set flag will be false,
 * and a new hedge_context_t will have to be created after the parameters are corrected.
 * 
 * By default, hedge_context_t creates a chain of hedge_context_t::ctxdata_t instances. The
 * first one in the chain corresponds to special encryption parameters that are reserved
 * to be used by the various key classes (SecretKey, PublicKey, etc.). These are the exact
 * same encryption parameters that are created by the user and passed to th constructor of
 * hedge_context_t. The functions key_context_data() and key_parms_id() return the ctxdata_t
 * and the parms_id corresponding to these special parameters. The rest of the ctxdata_t
 * instances in the chain correspond to encryption parameters that are derived from the
 * first encryption parameters by always removing the last one of the moduli in the
 * coeff_modulus, until the resulting parameters are no longer valid, e.g., there are no
 * more primes left. These derived encryption parameters are used by ciphertexts and
 * plaintexts and their respective ctxdata_t can be accessed through the
 * get_ctxdata(parms_id_type) function. The functions first_context_data() and
 * last_context_data() return the ctxdata_t corresponding to the first and the last
 * set of parameters in the "data" part of the chain, i.e., the second and the last element
 * in the full chain. The chain itself is a doubly linked list, and is referred to as the
 * modulus switching chain.
 * 
 * @see encrypt_parameters for more details on the parameters.
 * @see EncryptionParameterQualifiers for more details on the qualifiers.
 */
struct hedge_context {
    struct ctxdata *data;

    parms_id_type key_parms_id;
    parms_id_type first_parms_id;
    parms_id_type last_parms_id;
    sec_level_type sec_level;
    bool using_keyswitching;
    unordered_map(parms_id_type, ctxdata_ptr) *context_data_map;

    // add object that are allocated during the lifecycle of this class
    // all entries in this list will be freed in the del function
    list_head_t object_list;

    void (*init)(struct hedge_context *self);
    void (*init_with_parms)(struct hedge_context *self,
                            struct encrypt_parameters *parms,
                            bool expand_mod_chain, sec_level_type sec_level);
    void (*equal)(struct hedge_context *self, const struct hedge_context *assign);
    void (*free)(struct hedge_context *self);
    struct ctxdata *(*validate)(struct hedge_context *self,
                            struct encrypt_parameters *parms);
    struct ctxdata *(*create)(struct hedge_context *self,
                            const struct encrypt_parameters *parms,
                            bool expand_mod_chain,
                            sec_level_type sec_level);
    parms_id_type (*create_next_ctxdata)(struct hedge_context *self,
                            const parms_id_type *prev_parms);
    struct ctxdata *(*get_ctxdata)(struct hedge_context *self, parms_id_type *prev_parms_id);
    struct ctxdata *(*key_ctxdata)(struct hedge_context *self);
    struct ctxdata *(*first_ctxdata)(struct hedge_context *self);
    struct ctxdata *(*last_ctxdata)(struct hedge_context *self);
    bool (*is_parms_set)(struct hedge_context *self);
};

extern struct hedge_context *new_hcontext(
    encrypt_parameters* parms,
    bool expand_mod_chain,
    sec_level_type sec_level);
extern void del_hcontext(struct hedge_context *self);

#ifdef __cplusplus
}
#endif

#endif /* __CONTEXT_H__ */