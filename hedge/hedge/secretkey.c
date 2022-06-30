#include "secretkey.h"

static void load(SecretKey* self, bytestream_t* stream)
{
    self->sk_->load(self->sk_, stream);
}

static void save(SecretKey* self, bytestream_t* stream)
{
    self->sk_->save(self->sk_, stream);
}

static Plaintext* data(SecretKey* self)
{
    return self->sk_;
}

static parms_id_type* parms_id(SecretKey* self)
{
    return &self->sk_->parms_id_;
}

SecretKey* new_SecretKey(void)
{
    SecretKey* self = hedge_malloc(sizeof(SecretKey));

    self->sk_ = new_Plaintext();
    self->load = load;
    self->save = save;
    self->data = data;
    self->parms_id = parms_id;

    return self;
}

void del_SecretKey(SecretKey* sk)
{
    del_Plaintext(sk->data(sk));
    hedge_free(sk);
}