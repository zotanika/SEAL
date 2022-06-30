#include "ciphertext.h"
#include "math/polycore.h"
#include "lib/debug.h"
#include "hedge_malloc.h"
#include "encryptionparams.h"
#include "defines.h"

#define CIPHERTEXT_DEFAULT_SIZE 8192


static void reserve_internal(Ciphertext* self, size_type size_capacity,
    size_type poly_modulus_degree, size_type coeff_mod_count)
{
    if (size_capacity < HEDGE_CIPHERTEXT_SIZE_MIN ||
        size_capacity > HEDGE_CIPHERTEXT_SIZE_MAX)
    {
        invalid_argument("invalid size_capacity");
    }

    size_type new_data_capacity =
        mul_safe(size_capacity, poly_modulus_degree, coeff_mod_count);
    size_type new_data_size = min(new_data_capacity, self->data_->size(self->data_));

    // First reserve, then resize
    self->data_->reserve(self->data_, new_data_capacity);
    self->data_->resize(self->data_, new_data_size);

    // Set the size
    self->size_ = min(size_capacity, self->size_);
    self->poly_modulus_degree_ = poly_modulus_degree;

    self->coeff_mod_count_ = coeff_mod_count;
}

static void reserve(Ciphertext* self, hedge_context_t* context,
    parms_id_type* parms_id, size_type size_capacity)
{
    // Verify parameters
    if (!context)
    {
        invalid_argument("invalid context");
    }
    if (!context->is_parms_set(context))
    {
        invalid_argument("encryption parameters are not set correctly");
    }

    ctxdata_t* context_data_ptr = context->get_ctxdata(context, parms_id);
    if (!context_data_ptr)
    {
        invalid_argument("parms_id is not valid for encryption parameters");
    }

    // Need to set parms_id first
    encrypt_parameters* parms = context_data_ptr->parms; 
    self->parms_id_ = *context_data_ptr->parms->parms_id(context_data_ptr->parms);

    vector(Zmodulus)* sm = parms->coeff_modulus(parms);
    reserve_internal(self, size_capacity, parms->poly_modulus_degree(parms),
        (size_type)(sm->size));
}

static void resize_internal(Ciphertext* self, size_type size,
    size_type poly_modulus_degree, size_type coeff_mod_count)
{
    if ((size < HEDGE_CIPHERTEXT_SIZE_MIN && size != 0) ||
        size > HEDGE_CIPHERTEXT_SIZE_MAX)
    {
        invalid_argument("invalid size");
    }

    // Resize the data
    size_type new_data_size =
        mul_safe(size, poly_modulus_degree, coeff_mod_count);
    log_trace("before resize = %p (count=%zu) newsize=%zu\n", self->data_, self->data_->count, new_data_size);
    self->data_ = self->data_->resize(self->data_, new_data_size);
    log_trace("after resize = %p (count=%zu)\n", self->data_, self->data_->count);
    // Set the size parameters
    self->size_ = size;
    self->poly_modulus_degree_ = poly_modulus_degree;
    self->coeff_mod_count_ = coeff_mod_count;
}

static void resize(Ciphertext* self, hedge_context_t* context,parms_id_type* parms_id, size_type size)
{
    // Verify parameters
    if (!context)
    {
        invalid_argument("invalid context");
    }
    if (!context->is_parms_set(context))
    {
        invalid_argument("encryption parameters are not set correctly");
    }

    ctxdata_t* context_data_ptr = context->get_ctxdata(context, parms_id);
    if (!context_data_ptr)
    {
        invalid_argument("parms_id is not valid for encryption parameters");
    }

    // Need to set parms_id first
    encrypt_parameters* parms = context_data_ptr->parms;
    self->parms_id_ = *context_data_ptr->parms->parms_id(context_data_ptr->parms);

    vector(Zmodulus)* vector_sm = parms->coeff_modulus(parms);
    resize_internal(self, size, parms->poly_modulus_degree(parms),
        (size_type)(vector_sm->size));
}

static void save(Ciphertext* self, bytestream_t* stream)
{
    // reweind stream pos
    stream->reset(stream);

    stream->write(stream, (byte*)&self->parms_id_, sizeof(parms_id_type));
    stream->write(stream, (byte*)&self->is_ntt_form_, sizeof(byte));
    stream->write(stream, (byte*)&self->size_, sizeof(uint64_t));
    stream->write(stream, (byte*)&self->poly_modulus_degree_, sizeof(self->poly_modulus_degree_));
    stream->write(stream, (byte*)&self->coeff_mod_count_, sizeof(uint64_t));
    stream->write(stream, (byte*)&self->scale_, sizeof(double));

    log_trace("ciphertext->save: coeff_mod_count=%zu\n", self->coeff_mod_count_);
    self->data_->save(self->data_, stream);

    //print_polynomial(self->data_->data, self->data_->count, TRUE);
}

static void unsafe_load(Ciphertext* self, bytestream_t* stream)
{
    parms_id_type parms_id;
    byte is_ntt_form_byte;
    uint64_t size64 = 0;
    uint64_t poly_modulus_degree64 = 0;
    uint64_t coeff_mod_count64 = 0;
    double scale = 0;

    // rewind stream pos
    stream->reset(stream);

    stream->read(stream, (byte*)&parms_id, sizeof(parms_id_type));
    stream->read(stream, (byte*)&is_ntt_form_byte, sizeof(byte));
    stream->read(stream, (byte*)&size64, sizeof(uint64_t));
    stream->read(stream, (byte*)&poly_modulus_degree64, sizeof(uint64_t));
    stream->read(stream, (byte*)&coeff_mod_count64, sizeof(uint64_t));
    stream->read(stream, (byte*)&scale, sizeof(double));

    self->data_ = self->data_->load(self->data_, stream);

    self->parms_id_ = parms_id;
    self->is_ntt_form_ = (is_ntt_form_byte == 0) ? false : true;
    self->size_ = (size_type)size64;
    self->poly_modulus_degree_ = (size_type)poly_modulus_degree64;
    self->coeff_mod_count_ = (size_type)coeff_mod_count64;
    self->scale_ = scale;
}

static ct_coeff_type* data(Ciphertext* self)
{
    return self->data_->data;
}

static ct_coeff_type* at(Ciphertext* self, size_type poly_index)
{
    uint64_t poly_uint64_count = self->poly_modulus_degree_ * self->coeff_mod_count_;
    if (!poly_uint64_count) {
        return NULL;
    }
    if (poly_index >= self->size_) {
        out_of_range("poly_index must be within [0, size)");
    }
    return &self->data_->data[poly_index * poly_uint64_count];
}

static ct_coeff_type value_at(Ciphertext* self, size_type poly_index)
{
    uint64_t poly_uint64_count = self->poly_modulus_degree_ * self->coeff_mod_count_;
    if (!poly_uint64_count) {
        return 0;
    }
    if (poly_index >= self->size_) {
        out_of_range("poly_index must be within [0, size)");
    }
    return self->data_->data[poly_index * poly_uint64_count];
}

static size_type coeff_mod_count(Ciphertext* self)
{
    return self->coeff_mod_count_;
}

static size_type uint64_count_capacity(Ciphertext* self)
{
    return self->data_->capacity;
}
static size_type size_capacity(Ciphertext* self)
{
    size_type poly_uint64_count = self->poly_modulus_degree_ * self->coeff_mod_count_;
    return poly_uint64_count ?
        self->uint64_count_capacity(self) / poly_uint64_count : (size_type)0;
}
static size_type uint64_count(Ciphertext* self)
{
    return self->data_->size(self->data_);
}
static bool is_ntt_form(Ciphertext* self)
{
    return self->is_ntt_form_;
}
static parms_id_type* parms_id(Ciphertext* self)
{
    return &self->parms_id_;
}
static double scale(Ciphertext* self)
{
    return self->scale_;
}

static Ciphertext* assign(Ciphertext* self, Ciphertext* ciphertext)
{
    if (self == ciphertext)
        return self;
    
    self->parms_id_ = ciphertext->parms_id_;
    self->is_ntt_form_ = ciphertext->is_ntt_form_;
    self->scale_ = ciphertext->scale_;

    resize_internal(self, ciphertext->size_, ciphertext->poly_modulus_degree_,
            ciphertext->coeff_mod_count_);

    self->data_->copy(self->data_, ciphertext->data_);

    return self;
}

static size_type size(Ciphertext* self)
{
    return self->size_;
}

static void release(Ciphertext* self)
{
    self->parms_id_ = parms_id_zero;
}

Ciphertext* new_Ciphertext(void)
{
    Ciphertext* p = (Ciphertext *)hedge_zalloc(sizeof(Ciphertext));
    p->data_ = new_array(CIPHERTEXT_DEFAULT_SIZE);
    p->reserve = reserve;
    p->reserve_internal = reserve_internal;
    p->resize = resize;
    p->size = size;
    p->release = release;
    p->data = data;
    p->at = at;
    p->value_at = value_at;
    p->coeff_mod_count = coeff_mod_count;
    p->uint64_count_capacity = uint64_count_capacity;
    p->size_capacity = size_capacity;
    p->uint64_count = uint64_count;
    p->is_ntt_form = is_ntt_form;
    p->parms_id = parms_id;
    p->scale = scale;
    p->save = save;
    p->load = unsafe_load;
    p->assign = assign;

    return p;
}

void del_Ciphertext(Ciphertext* ciphertext)
{
    hedge_free(ciphertext->data_);
    hedge_free(ciphertext);
}

