#ifndef __PUBLICKEY_H__
#define __PUBLICKEY_H__
#include "lib/types.h"
#include "ciphertext.h"
#include "lib/bytestream.h"
typedef struct PublicKey PublicKey;
struct PublicKey {
    void (*load)(PublicKey* self, bytestream_t* stream);
    void (*save)(PublicKey* self, bytestream_t* stream);
    void (*assign)(PublicKey* self, PublicKey* src);
    Ciphertext* (*data)(PublicKey* self);
    parms_id_type* (*parms_id)(PublicKey* self);
    
    Ciphertext* pk_;
};

PublicKey* new_PublicKey(void);
void build_publickey(PublicKey *pk);
void del_PublicKey(PublicKey* pk);

#endif /* __PUBLICKEY_H__ */