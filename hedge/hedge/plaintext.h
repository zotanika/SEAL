#ifndef __PLAINTEXT_H__
#define __PLAINTEXT_H__

#include "lib/types.h"
#include "lib/bytestream.h"
#include "encryptionparams.h"
#include "lib/array.h"
typedef struct Plaintext Plaintext;

typedef uint64_t pt_coeff_type;
typedef size_t size_type;

struct Plaintext {
    parms_id_type parms_id_;
    double scale_;
    array_t *data_;

    double (*scale)(Plaintext* self);
    parms_id_type* (*parms_id)(Plaintext* self);
    void (*reserve)(Plaintext* self, size_type capacity);
    void (*shrink_to_fit)(Plaintext* self);
    void (*release)(Plaintext* self);
    void (*resize)(Plaintext* self, size_type coeff_count);
    void (*set_zero)(Plaintext* self, size_type start_coeff, size_type length);
    pt_coeff_type* (*data)(Plaintext* self);
    pt_coeff_type* (*at)(Plaintext* self, size_type coeff_index);
    pt_coeff_type (*value_at)(Plaintext* self, size_type coeff_index);
    bool (*is_equal)(Plaintext* self, const Plaintext *compare);
    bool (*is_zero)(Plaintext* self);
    size_type (*capacity)(Plaintext* self);
    size_type (*coeff_count)(Plaintext* self);
    size_type (*significant_coeff_count)(Plaintext* self);
    size_type (*nonzero_coeff_count)(Plaintext* self);
    char* (*to_string)(Plaintext* self); // free returned string after use!
    void (*save)(Plaintext* self, bytestream_t* stream);
    void (*load)(Plaintext* self, bytestream_t* stream);
    bool (*is_ntt_form)(Plaintext* self);
};

Plaintext* new_Plaintext(void);
void del_Plaintext(Plaintext* plaintext);

#endif /* __PLAINTEXT_H__ */