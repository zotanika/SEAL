#ifndef __CKKS_H__
#define __CKKS_H__

#include <complex.h>
#include <memory.h>
//#include <type_traits>
//#include <cmath.h>
#include "lib/vector.h"
#include <limits.h>
#include "plaintext.h"
#include "context.h"
#include "lib/common.h"
#include "math/uintcore.h"
//#include "math/uintarithsmallmod.h"

typedef struct CKKSEncoder CKKSEncoder;

#define PI_ 3.14159265358979323846

    struct CKKSEncoder {
        hedge_context_t* context_;
        size_t slots_;
        double complex* roots_;
        double complex* inv_roots_;
        uint64_t* matrix_reps_index_map_;

        void (*encode)(CKKSEncoder* self, vector(double_complex)* values,
            parms_id_type* parms_id, double scale, Plaintext* destination);
        void (*encode_simple)(CKKSEncoder* self, double* array, size_t count,
            double scale, Plaintext* destination);
        void (*decode)(CKKSEncoder* self, Plaintext* plain, vector(double_complex)* destination);
        size_t (*slot_count)(CKKSEncoder* self);
};

CKKSEncoder* new_CKKSEncoder(hedge_context_t* context);
void del_CKKSEncoder(CKKSEncoder* self);

#endif /* __CKKS_H__ */