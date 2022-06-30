#ifndef __SECRETKEY_H__
#define __SECRETKEY_H__

#include "lib/types.h"
#include "plaintext.h"
#include "lib/bytestream.h"

typedef struct SecretKey SecretKey;
struct SecretKey {
    void (*load)(SecretKey* self, bytestream_t* stream);
    void (*save)(SecretKey* self, bytestream_t* stream);
    Plaintext* (*data)(SecretKey* self);
    parms_id_type* (*parms_id)(SecretKey* self);
    Plaintext* sk_;
};

SecretKey* new_SecretKey(void);
void del_SecretKey(SecretKey* pk);

#endif /* __SECRETKEY_H__ */
