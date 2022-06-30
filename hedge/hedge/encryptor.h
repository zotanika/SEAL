#ifndef __ENCRYPTOR_H__
#define __ENCRYPTOR_H__

#include "encryptionparams.h"
#include "plaintext.h"
#include "ciphertext.h"
#include "context.h"
#include "publickey.h"
#include "lib/vector.h"
#include "hedge_malloc.h"
#include "math/smallntt.h"

typedef struct Encryptor Encryptor;

struct Encryptor {
    void (*encrypt_zero)(Encryptor* self, parms_id_type* parms_id, Ciphertext* destination);
    void (*encrypt)(Encryptor* self, const Plaintext* plain, Ciphertext* destination);
    hedge_context_t* context_;
    PublicKey* public_key_;
    struct rng random;
};

Encryptor* new_Encryptor(hedge_context_t* context, PublicKey* pubkey);
void del_Encryptor(Encryptor* self);

#endif
