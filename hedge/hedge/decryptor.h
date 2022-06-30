#ifndef __DECRYPTOR_H__
#define __DECRYPTOR_H__

#include "math/rng.h"
#include "encryptionparams.h"
#include "context.h"
#include "math/smallntt.h"
#include "hedge_malloc.h"
#include "ciphertext.h"
#include "plaintext.h"
#include "secretkey.h"
#include "math/baseconverter.h"
#include "math/smallmodulus.h"

typedef struct Decryptor Decryptor;
struct Decryptor
{
    void (*decrypt)(Decryptor* self, const Ciphertext *encrypted,
        Plaintext *destination);
    int (*invariant_noise_budget)(Decryptor* self, const Ciphertext *encrypted);
    void (*ckks_decrypt)(Decryptor* self, const Ciphertext *encrypted,
        Plaintext *destination);
    void (*compute_secret_key_array)(Decryptor* self, size_t max_power);
    void (*compose)(Decryptor* self, const ctxdata_t *context_data, uint64_t *value);

    hedge_context_t* context_;
    size_t secret_key_array_size_;
    uint64_t* secret_key_array_;
};

Decryptor* new_Decryptor(hedge_context_t* context, SecretKey* secret_key);
void del_Decryptor(Decryptor* self);

#endif /* __DECRYPTOR_H__ */