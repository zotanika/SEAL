#ifndef __HEDGE_H__
#define __HEDGE_H__

#include "lib/types.h"

struct object {
    size_t size;
    byte data[];
};
typedef struct object object_t;

typedef struct object hedge_ciphertext_t;
typedef struct object hedge_plaintext_t;
typedef struct object hedge_publickey_t;
typedef struct object hedge_secretkey_t;
typedef struct object hedge_relinkeys_t;

struct double_array {
    size_t size;
    double data[];
};

struct int_array {
    size_t size;
    int data[];
};

struct hedge_config {
    uint64 poly_modulus_degree; 
    uint64 coeff_modulus;
    double scale;
};

typedef struct double_array hedge_double_array_t;
typedef struct hedge_config hedge_config_t;
typedef struct hedge_internals hedge_internals_t;
typedef struct hedge hedge_t;

struct hedge_internals {
    void* parms;
    void* context;
    void* keygen;
    void* pubkey;
    void* seckey;
    void* relinkeys;
    void* encryptor;
    void* evaluator;
    void* encoder;
    void* decryptor;
    // add object that are allocated during the lifecycle of the hedge instance
    // all entries in this list will be freed in the shred function
    void* object_list; /* list_head_t */
};

struct hedge {
    hedge_config_t config;
    hedge_publickey_t* pubkey;
    hedge_secretkey_t* seckey;
    hedge_relinkeys_t* relinkeys;
    hedge_internals_t internals;
};

enum {
    HEDGE_COMPAT_TEST_LOGGING = 0,
    HEDGE_COMPAT_TEST_MALLOC = 1,
    HEDGE_COMPAT_TEST_IO = 2,
};

void hedge_api_compat_test(int testnum, unsigned long arg0, unsigned long arg1);
int hedge_set_log_level(int log_level);

hedge_t* hedge_create(hedge_config_t* config);

hedge_ciphertext_t* hedge_encrypt(hedge_t* self, double* array, size_t len);
hedge_double_array_t* hedge_decrypt(hedge_t* self, hedge_ciphertext_t* ciphertext);

hedge_plaintext_t* hedge_encode(hedge_t* hedge, double* array, size_t len);
hedge_double_array_t* hedge_decode(hedge_t* hedge, hedge_plaintext_t* plaintext);

void hedge_shred(hedge_t* self);

#endif /* __HEDGE_H__ */