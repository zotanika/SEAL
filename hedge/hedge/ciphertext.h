#ifndef __CIPHERTEXT_H__
#define __CIPHERTEXT_H__

#include "lib/array.h"
#include "encryptionparams.h"
#include "context.h"

/* forward declaration */
typedef struct Ciphertext Ciphertext;

typedef uint64_t ct_coeff_type;
typedef size_t size_type;

struct Ciphertext {
    parms_id_type parms_id_;
    bool is_ntt_form_;
    size_type size_;
    size_type poly_modulus_degree_;
    size_type coeff_mod_count_;
    double scale_;
    array_t* data_;

    void (*reserve)(Ciphertext* self, hedge_context_t* context, parms_id_type* parms_id, size_type size_capacity);
    void (*reserve_internal)(Ciphertext* self, size_type size_capacity,
            size_type poly_modulus_degree, size_type coeff_mod_count);
    size_type (*size)(Ciphertext* self);
    void (*resize)(Ciphertext* self, hedge_context_t* context,parms_id_type* parms_id, size_type size);
    void (*release)(Ciphertext* self);
    ct_coeff_type* (*data)(Ciphertext* self);
    ct_coeff_type* (*at)(Ciphertext* self, size_type poly_index);
    ct_coeff_type (*value_at)(Ciphertext* self, size_type poly_index);
    size_type (*coeff_mod_count)(Ciphertext* self);
    size_type (*uint64_count_capacity)(Ciphertext* self);
    size_type (*size_capacity)(Ciphertext* self);
    size_type (*uint64_count)(Ciphertext* self);
    bool (*is_ntt_form)(Ciphertext* self);
    parms_id_type* (*parms_id)(Ciphertext* self);
    double (*scale)(Ciphertext* self);
    void (*save)(Ciphertext* self, bytestream_t* stream);
    void (*load)(Ciphertext* self, bytestream_t* stream);

    // operators
    Ciphertext* (*assign)(Ciphertext* self, Ciphertext* ciphertext);

};

Ciphertext* new_Ciphertext(void);
void del_Ciphertext(Ciphertext* ciphertext);

#endif /* __CIPHERTEXT_H__ */