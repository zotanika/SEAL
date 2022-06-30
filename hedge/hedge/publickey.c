#include "publickey.h"

static void load(PublicKey* self, bytestream_t* stream)
{
    self->pk_->load(self->pk_, stream);
}

static void save(PublicKey* self, bytestream_t* stream)
{
    self->pk_->save(self->pk_, stream);
}

static void assign(PublicKey* self, PublicKey* src)
{
    self->pk_->assign(self->pk_, src->pk_);
}

static Ciphertext* data(PublicKey* self)
{
    return self->pk_;
}

static parms_id_type* parms_id(PublicKey* self)
{
    return &self->pk_->parms_id_;
}

PublicKey* new_PublicKey(void)
{
    PublicKey* self = hedge_malloc(sizeof(PublicKey));

    self->pk_ = new_Ciphertext();
    self->load = load;
    self->save = save;
    self->assign = assign;
    self->data = data;
    self->parms_id = parms_id;

    return self;
}

void build_publickey(PublicKey *pk)
{
    assert(pk);
    
    pk->pk_ = new_Ciphertext();
    pk->load = load;
    pk->save = save;
    pk->assign = assign;
    pk->data = data;
    pk->parms_id = parms_id;
}

void del_PublicKey(PublicKey* pk)
{
    del_Ciphertext(pk->data(pk));
    hedge_free(pk);
}