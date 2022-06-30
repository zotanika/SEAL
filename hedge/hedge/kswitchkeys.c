#include <assert.h>
#include "valcheck.h"
#include "kswitchkeys.h"

static void assign(struct kswitch_keys *self, struct kswitch_keys *src)
{
    size_t i, j;

    if (self == src)
        return;

    self->keys->clear(self->keys);
 
    for (i = 0; i < src->keys->size; i++) {
        vector_publickey *this = self->keys->at(self->keys, i);
        vector_publickey *that = src->keys->at(src->keys, i);
        for (j = 0; j < this->size; j++) {
            PublicKey *new = new_PublicKey();
            new->assign(new, that->at(that, j));
            this->emplace_back(this, new);
        }
    }
}

static void save_kswitchkeys(struct kswitch_keys *self, const bytestream_t *stream)
{
    uint64_t keys_dim1, keys_dim2;
    size_t i, j;

    assert(stream);

    keys_dim1 = (uint64_t)self->keys->size;
    stream->write((bytestream_t*)stream, (byte *)self->parms_id, sizeof(parms_id_type *));
    stream->write((bytestream_t*)stream, (byte *)&keys_dim1, sizeof(uint64_t));

    for (i = 0; i < keys_dim1; i++) {
        vector_publickey *this = self->keys->at(self->keys, i);
        keys_dim2 = (uint64_t)this->size;
        stream->write((bytestream_t*)stream, (byte *)&keys_dim2, sizeof(uint64_t));

        for (j = 0; j < keys_dim2; j++) {
            PublicKey *end = this->at(this, j);
            end->save(end, (bytestream_t *)stream);
        }
    }
}

static void unsafe_load_kswitchkeys(
        struct kswitch_keys *self,
        bytestream_t *stream)
{
    uint64_t keys_dim1, keys_dim2;
    size_t i, j;

    vector(vector_publickey) *new_keys;

    assert(stream);

    stream->read(stream, (byte *)self->parms_id, sizeof(parms_id_type *));
    stream->read(stream, (byte *)&keys_dim1, sizeof(uint64_t));

    new_keys = new_vector(vector_publickey, (size_t)keys_dim1);
    for (i = 0; i < keys_dim1; i++) {
        vector(PublicKey) *this = new_keys->at(new_keys, i);
        stream->read(stream, (byte *)&keys_dim2, sizeof(uint64_t));

        new_keys->emplace_back(new_keys, this);
        for (j = 0; j < keys_dim2; j++) {
            PublicKey *key = new_PublicKey();
            key->load(key, stream);
            this->emplace_back(this, key);
        }
    }

    if (self->keys) {
        del_vector(self->keys);
        self->keys = NULL;
    }
    self->keys = new_keys;
}

static void load_kswitchkeys(
        struct kswitch_keys *self,
        struct hedge_context *context,
        bytestream_t *stream)
{
    self->unsafe_load(self, stream);

    assert(is_valid_for(KSwitchKeys, self, context));
}

static size_t kswitchkeys_size(struct kswitch_keys *self)
{
    size_t res = 0;
    size_t i;

    for (i = 0; i < self->keys->size; i++) {
        vector_publickey *this = self->keys->at(self->keys, i);
        if (this->size > 0)
            res++;
        else
            break;
    }
    return res;
}

static vector_publickey *kswitchkeys_data(struct kswitch_keys *self, size_t index)
{
    vector_publickey *element = self->keys->at(self->keys, index);
    assert(index < self->keys->size && element->size > 0);
    return element;
}

static void init_kswitchkeys(struct kswitch_keys *self)
{
    self->parms_id = &parms_id_zero;
    self->keys = new_vector(vector_publickey, 1);
    vector_publickey *vec = new_vector(PublicKey, 1);
    self->keys->data_swap(self->keys, vec->size, vec);
}

static void free_kswitchkeys(struct kswitch_keys *self)
{
    if (self->keys) {
        vector_publickey* vec;
        PublicKey* pk;
        size_t i, j;
        iterate_vector(i, vec, self->keys) {
            iterate_vector(j, pk, vec) {
                del_Ciphertext(pk->data(pk));
            }
        }
        del_vector(self->keys);
        self->keys = NULL;
    }
}

DECLARE_VECTOR_BODY(PublicKey);
DECLARE_VECTOR_BODY(vector_publickey);

struct kswitch_keys *new_kswitchkeys(void)
{
    struct kswitch_keys *inst = hedge_zalloc(sizeof(struct kswitch_keys));
    if (!inst)
        return NULL;

    *inst = (struct kswitch_keys) {
        .init = init_kswitchkeys,
        .assign = assign,
        .free = free_kswitchkeys,
        .size = kswitchkeys_size,
        .data = kswitchkeys_data,
        .save = save_kswitchkeys,
        .unsafe_load = unsafe_load_kswitchkeys,
        .load = load_kswitchkeys
    };
    inst->init(inst);
    return inst;
}

void del_kswitchkeys(struct kswitch_keys *self)
{
    if (self->free)
        self->free(self);
    hedge_free(self);
}