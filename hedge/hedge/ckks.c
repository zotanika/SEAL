#include "ckks.h"
#include <math.h>
#include "math/smallmodulus.h"
#include "math/baseconverter.h"
#include "lib/algorithm.h"
#include "lib/common.h"
#include "valcheck.h"
#include "math/uintarithsmallmod.h"
#include "math/uintarithmod.h"

static size_t slot_count(CKKSEncoder* self)
{
    return self->slots_;
}

static void decompose_single_coeff(const ctxdata_t* context_data, const uint64_t *value,
            uint64_t *destination)
{
    encrypt_parameters* parms = context_data->parms;
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus(parms);
    size_t coeff_mod_count = coeff_modulus->size;
#ifdef HEDGE_DEBUG
    if (value == nullptr)
    {
        invalid_argument("value cannot be null");
    }
    if (destination == nullptr)
    {
        invalid_argument("destination cannot be null");
    }
    if (destination == value)
    {
        invalid_argument("value cannot be the same as destination");
    }
#endif
    if (coeff_mod_count == 1)
    {
        set_uint_uint(value, coeff_mod_count, destination);
        return;
    }

    uint64_t* value_copy = hedge_malloc(sizeof(uint64_t) * coeff_mod_count);
    for (size_t j = 0; j < coeff_mod_count; j++)
    {
        //destination[j] = modulo_uint(
        //    value, coeff_mod_count, coeff_modulus_[j], pool);

        // Manually inlined for efficiency
        // Make a fresh copy of value
        set_uint_uint(value, coeff_mod_count, value_copy);

        // Starting from the top, reduce always 128-bit blocks
        for (size_t k = coeff_mod_count - 1; k--; )
        {
            value_copy[k] = barrett_reduce_128(
                value_copy + k, coeff_modulus->at(coeff_modulus, j));
        }
        destination[j] = value_copy[0];
    }
    hedge_free(value_copy);
}

static void encode_internal(CKKSEncoder* self, vector(double_complex)* values,
                parms_id_type* parms_id, double scale, Plaintext* destination)
{
    // Verify parameters.
    ctxdata_t* context_data_ptr = self->context_->get_ctxdata(self->context_, parms_id);
    if (!context_data_ptr)
    {
        invalid_argument("parms_id is not valid for encryption parameters");
    }
    if (values->size > self->slots_)
    {
        invalid_argument("values has invalid size");
    }
    
    log_trace("encode_internal: %lu %lu %lu %lu\n", 
        parms_id->data[0], parms_id->data[1],
        parms_id->data[2], parms_id->data[3]);

    ctxdata_t* context_data = context_data_ptr;
    encrypt_parameters* parms = context_data->parms;
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus(parms);
    size_t coeff_mod_count = coeff_modulus->size;
    size_t coeff_count = parms->poly_modulus_degree(parms);

    // Quick sanity check
    if (!product_fits_in(coeff_mod_count, coeff_count))
    {
        logic_error("invalid parameters");
    }

    // Check that scale is positive and not too large
    if (scale <= 0 || ((int)(log2(scale)) + 1 >=
        context_data->total_coeff_modulus_bit_count))
    {
        invalid_argument("scale out of bounds");
    }

    SmallNTTTables** small_ntt_tables = context_data->small_ntt_tables;

    // input_size is guaranteed to be no bigger than slots_
    size_t input_size = values->size;
    size_t n = mul_safe(self->slots_, (size_t)(2));

    double complex* conj_values = hedge_zalloc(sizeof(double complex) * n);

    log_trace("encode_internal %lf\n", creal(values->value_at(values, 0)));
    for (size_t i = 0; i < input_size; i++)
    {
        conj_values[self->matrix_reps_index_map_[i]] = values->value_at(values,i);
        conj_values[self->matrix_reps_index_map_[i + self->slots_]] = conj(values->value_at(values,i));
        log_trace("@@!!conj_values[matrix_reps_index_map_[i]]=%lf\n", creal(conj_values[self->matrix_reps_index_map_[i]]));
    }

    int logn = get_power_of_two(n);
    size_t tt = 1;
    for (int i = 0; i < logn; i++)
    {
        size_t mm = (size_t)(1) << (logn - i);
        size_t k_start = 0;
        size_t h = mm / 2;

        for (size_t j = 0; j < h; j++)
        {
            size_t k_end = k_start + tt;
            double complex s = self->inv_roots_[h + j];

            for (size_t k = k_start; k < k_end; k++)
            {
                double complex u = conj_values[k];
                double complex v = conj_values[k + tt];
                conj_values[k] = u + v;
                conj_values[k + tt] = (u - v) * s;
            }

            k_start += 2 * tt;
        }
        tt *= 2;
    }

    double n_inv = (double)(1.0) / (double)(n);

    // Put the scale in at this point
    n_inv *= scale;

    int max_coeff_bit_count = 1;
    for (size_t i = 0; i < n; i++)
    {
        // Multiply by scale and n_inv (see above)
        conj_values[i] *= n_inv;
        if (i < 3)
            log_trace("@@!!conj_values[%zu]=%lf\n", creal(conj_values[i]));
        // Verify that the values are not too large to fit in coeff_modulus
        // Note that we have an extra + 1 for the sign bit
        max_coeff_bit_count = max(max_coeff_bit_count,
            (int)(log2(fabs(creal(conj_values[i])))) + 2);
    }
    if (max_coeff_bit_count >= context_data->total_coeff_modulus_bit_count)
    {
        invalid_argument("encoded values are too large");
    }

    double two_pow_64 = pow(2.0, 64);

    // Resize destination to appropriate size
    // Need to first set parms_id to zero, otherwise resize
    // will throw an exception.
    destination->parms_id_ = parms_id_zero;
    destination->resize(destination, mul_safe(coeff_count, coeff_mod_count));

    log_trace("max_coeff_bit_count=%lu\n", max_coeff_bit_count);
    // Use faster decomposition methods when possible
    if (max_coeff_bit_count <= 64)
    {
        for (size_t i = 0; i < n; i++)
        {
            double coeffd = round(creal(conj_values[i]));
            bool is_negative = signbit(coeffd);
            uint64_t coeffu = (uint64_t)(fabs(coeffd));
            if (i < 3)
                log_trace("@@@@!! coeffd=%lf is_negative=%d coeffu=%zu\n",
                    coeffd, is_negative, coeffu);
            if (is_negative)
            {
                for (size_t j = 0; j < coeff_mod_count; j++)
                {
                    *destination->at(destination, i + (j * coeff_count)) = negate_uint_smallmod(
                        coeffu % coeff_modulus->at(coeff_modulus, j)->value,
                        coeff_modulus->at(coeff_modulus, j));
                }
            }
            else
            {
                for (size_t j = 0; j < coeff_mod_count; j++)
                {
                    *destination->at(destination, i + (j * coeff_count)) =
                        coeffu % coeff_modulus->at(coeff_modulus, j)->value;
                    if (i + (j*coeff_count) < 10)
                        log_trace("##@@ destination[%zu]=%zu\n", i + (j*coeff_count),
                            *destination->at(destination, i + (j * coeff_count)));
                }
            }
        }
    }
    else if (max_coeff_bit_count <= 128)
    {
        for (size_t i = 0; i < n; i++)
        {
            double coeffd = round(creal(conj_values[i]));
            bool is_negative = signbit(coeffd);
            coeffd = fabs(coeffd);

            uint64_t coeffu[2] = {
                (uint64_t)(fmod(coeffd, two_pow_64)),
                (uint64_t)(coeffd / two_pow_64) };

            if (is_negative)
            {
                for (size_t j = 0; j < coeff_mod_count; j++)
                {
                    *destination->at(destination, i + (j * coeff_count)) =
                        negate_uint_smallmod(barrett_reduce_128(
                            coeffu, coeff_modulus->at(coeff_modulus, j)),
                            coeff_modulus->at(coeff_modulus, j));
                }
            }
            else
            {
                for (size_t j = 0; j < coeff_mod_count; j++)
                {
                    *destination->at(destination, i + (j * coeff_count)) =
                        barrett_reduce_128(coeffu, coeff_modulus->at(coeff_modulus, j));
                }
            }
        }
    }
    else
    {
        log_fatal("@@@@@@@ SLOW CASW!!!!!\n");
        // Slow case
        uint64_t* coeffu = hedge_zalloc(sizeof(uint64_t) * coeff_mod_count);
        uint64_t* decomp_coeffu = hedge_zalloc(sizeof(uint64_t) * coeff_mod_count);
        
        for (size_t i = 0; i < n; i++)
        {
            double coeffd = round(creal(conj_values[i]));
            bool is_negative = signbit(coeffd);
            coeffd = fabs(coeffd);

            // We are at this point guaranteed to fit in the allocated space
            set_zero_uint(coeff_mod_count, coeffu);
            uint64_t* coeffu_ptr = coeffu;
            while (coeffd >= 1)
            {
                *coeffu_ptr++ = (uint64_t)(fmod(coeffd, two_pow_64));
                coeffd /= two_pow_64;
            }

            // Next decompose this coefficient
            decompose_single_coeff(context_data, coeffu,
                decomp_coeffu);

            // Finally replace the sign if necessary
            if (is_negative)
            {
                for (size_t j = 0; j < coeff_mod_count; j++)
                {
                    *destination->at(destination, i + (j * coeff_count)) =
                        negate_uint_smallmod(decomp_coeffu[j], coeff_modulus->at(coeff_modulus, j));
                        log_trace("decomp_coeffu[%zu] = %lu\n", j, decomp_coeffu[j]);
                }
            }
            else
            {
                for (size_t j = 0; j < coeff_mod_count; j++)
                {
                    *destination->at(destination, i + (j * coeff_count)) =
                        decomp_coeffu[j];
                    log_trace("decomp_coeffu[%zu] = %lu\n", j, decomp_coeffu[j]);
                }
            }
        }
        hedge_free(coeffu);
        hedge_free(decomp_coeffu);
    }

    // Transform to NTT domain
    for (size_t i = 0; i < coeff_mod_count; i++)
    {
        log_trace("Before NTT transform: %zu\n", *destination->at(destination, i * coeff_count));
        ntt_negacyclic_harvey(
            destination->at(destination, i * coeff_count), small_ntt_tables[i]);
        log_trace("NTT domain: %zu\n", *destination->at(destination, i * coeff_count));
    }

    destination->parms_id_ = *parms_id;
    destination->scale_ = scale;

    //print_polynomial("ckks plaintext", destination->data_->data, destination->data_->count, TRUE);
    hedge_free(conj_values);
}

static void encode_internal_int64(CKKSEncoder* self, int64_t value, parms_id_type* parms_id,
    Plaintext* destination)
{
    // Verify parameters.
    ctxdata_t* context_data_ptr = self->context_->get_ctxdata(self->context_, parms_id);
    if (!context_data_ptr)
    {
        invalid_argument("parms_id is not valid for encryption parameters");
    }

    ctxdata_t* context_data = context_data_ptr;
    encrypt_parameters* parms = context_data->parms;
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus(parms);
    size_t coeff_mod_count = coeff_modulus->size;
    size_t coeff_count = parms->poly_modulus_degree(parms);

    // Quick sanity check
    if (!product_fits_in(coeff_mod_count, coeff_count))
    {
        logic_error("invalid parameters");
    }

    int coeff_bit_count = get_significant_bit_count(
        (uint64_t)(llabs(value))) + 2;
    if (coeff_bit_count >= context_data->total_coeff_modulus_bit_count)
    {
         invalid_argument("encoded value is too large");
    }

    // Resize destination to appropriate size
    // Need to first set parms_id to zero, otherwise resize
    // will throw an exception.
    destination->parms_id_ = parms_id_zero;
    destination->resize(destination, coeff_count * coeff_mod_count);

    if (value < 0)
    {
        for (size_t j = 0; j < coeff_mod_count; j++)
        {
            uint64_t tmp = (uint64_t)(value);
            Zmodulus* sm = coeff_modulus->at(coeff_modulus, j);
            tmp += sm->value;
            tmp %= sm->value;
            fill_n(destination->data(destination) + (j * coeff_count), coeff_count, tmp);
        }
    }
    else
    {
        for (size_t j = 0; j < coeff_mod_count; j++)
        {
            uint64_t tmp = (uint64_t)value;
            Zmodulus* sm = coeff_modulus->at(coeff_modulus, j);
            tmp %= sm->value;
            fill_n(destination->data(destination) + (j * coeff_count), coeff_count, tmp);
        }
    }

    destination->parms_id_ = *parms_id;
    destination->scale_ = 1.0;
}

static void encode(CKKSEncoder* self, vector(double_complex)* values,
            parms_id_type* parms_id, double scale, Plaintext* destination)
{
    if (scale <= 0) scale = 1.0;
    encode_internal(self, values, parms_id, scale, destination);
}

static void encode_simple(CKKSEncoder* self, double* array, size_t count,
            double scale, Plaintext* destination)
{
    vector(double_complex)* values = new_vector(double_complex, count);
    values->resize(values, count);
    size_t i;
    double_complex* val;
    iterate_vector(i, val, values) {
        *val = array[i];
    }

    encode(self, values, &self->context_->first_parms_id, scale, destination);
    del_vector(values);
}

void decode_internal(CKKSEncoder* self, Plaintext* plain, 
                vector(double_complex)* destination)
{
    // Verify parameters.
    if (!is_valid_for(Plaintext, plain, self->context_))
    {
        invalid_argument("plain is not valid for encryption parameters");
    }
    if (!plain->is_ntt_form(plain))
    {
        invalid_argument("plain is not in NTT form");
    }

    ctxdata_t*  context_data_ptr = self->context_->get_ctxdata(self->context_, plain->parms_id(plain));
    encrypt_parameters* parms = context_data_ptr->parms;
    vector(Zmodulus)* coeff_modulus = parms->coeff_modulus(parms);
    size_t coeff_mod_count = coeff_modulus->size;
    size_t coeff_count = parms->poly_modulus_degree(parms);
    size_t rns_poly_uint64_count =
        mul_safe(coeff_count, coeff_mod_count);

    SmallNTTTables** small_ntt_tables = context_data_ptr->small_ntt_tables;

    // Check that scale is positive and not too large
    if (plain->scale(plain) <= 0 || ((int)(log2(plain->scale(plain))) >=
        context_data_ptr->total_coeff_modulus_bit_count))
    {
        invalid_argument("scale out of bounds");
    }

    uint64_t* decryption_modulus = context_data_ptr->total_coeff_modulus;
    uint64_t* upper_half_threshold = context_data_ptr->upper_half_threshold;

    struct base_converter* bc = context_data_ptr->base_converter;
    uint64_t* inv_coeff_products_mod_coeff_array =
        bc->inv_coeff_base_products_mod_coeff_array;
    uint64_t* coeff_products_array =
        bc->coeff_products_array;

    int logn = get_power_of_two(coeff_count);

    // Quick sanity check
    if ((logn < 0) || (coeff_count < HEDGE_POLY_MOD_DEGREE_MIN) ||
        (coeff_count > HEDGE_POLY_MOD_DEGREE_MAX))
    {
        logic_error("invalid parameters");
    }
    log_trace("coeff_count=%lu\n", coeff_count);
    double inv_scale = (double)(1.0) / plain->scale(plain);

    // Create mutable copy of input
    uint64_t* plain_copy = hedge_malloc(sizeof(uint64_t) * rns_poly_uint64_count);
    set_uint_uint(plain->data(plain), rns_poly_uint64_count, plain_copy);

    // Array to keep number bigger than std::uint64_t
    uint64_t* temp = hedge_malloc(sizeof(uint64_t) * coeff_mod_count);

    // destination mod q
    uint64_t* wide_tmp_dest = hedge_zalloc(sizeof(uint64_t) * rns_poly_uint64_count);


    // Transform each polynomial from NTT domain
    for (size_t i = 0; i < coeff_mod_count; i++)
    {
        inverse_ntt_negacyclic_harvey(
            plain_copy + (i * coeff_count), small_ntt_tables[i]);
    }

    log_trace("decode_internal : coeff_count = %zu\n", coeff_count);
    log_trace("decode_internal : coeff_mod_count = %zu\n", coeff_mod_count);
    double complex* res = hedge_malloc(sizeof(double complex) * coeff_count);

    double two_pow_64 = pow(2.0, 64);
    for (size_t i = 0; i < coeff_count; i++)
    {
        for (size_t j = 0; j < coeff_mod_count; j++)
        {
            uint64_t tmp = multiply_uint_uint_smallmod(
                plain_copy[(j * coeff_count) + i],
                inv_coeff_products_mod_coeff_array[j], // (qi/q * plain[i]) mod qi
                coeff_modulus->at(coeff_modulus, j));
            multiply_uint_uint64(
                coeff_products_array + (j * coeff_mod_count),
                coeff_mod_count, tmp, coeff_mod_count, temp);
            add_uint_uint_mod(temp,
                wide_tmp_dest + (i * coeff_mod_count),
                decryption_modulus, coeff_mod_count,
                wide_tmp_dest + (i * coeff_mod_count));
            //log_fatal("wide_tmp_dest[%d*%d]=%lu\n", i, coeff_mod_count, wide_tmp_dest[i*coeff_mod_count]);

        }

        res[i] = 0.0;
        if (is_greater_than_or_equal_uint_uint(
            wide_tmp_dest + (i * coeff_mod_count),
            upper_half_threshold, coeff_mod_count))
        {
            //log_fatal("wide_tmp_dest is greater than upper_half : %d\n", i);
            double scaled_two_pow_64 = inv_scale;
            for (size_t j = 0; j < coeff_mod_count;
                j++, scaled_two_pow_64 *= two_pow_64)
            {
                if (wide_tmp_dest[i * coeff_mod_count + j] > decryption_modulus[j])
                {
                    uint64_t diff = wide_tmp_dest[i * coeff_mod_count + j] - decryption_modulus[j];
                    res[i] += diff ?
                        (double)(diff) * scaled_two_pow_64 : 0.0;
                }
                else
                {
                    uint64_t diff = decryption_modulus[j] - wide_tmp_dest[i * coeff_mod_count + j];
                    res[i] -= diff ?
                        (double)(diff) * scaled_two_pow_64 : 0.0;
                }
            }
        }
        else
        {
            //log_fatal("wide_tmp_dest is NOT greater than upper_half : %d\n", i);
            double scaled_two_pow_64 = inv_scale;
            for (size_t j = 0; j < coeff_mod_count;
                j++, scaled_two_pow_64 *= two_pow_64)
            {
                uint64_t curr_coeff = wide_tmp_dest[i * coeff_mod_count + j];
                res[i] += curr_coeff ?
                    (double)(curr_coeff) * scaled_two_pow_64 : 0.0;
            }
        }

        // Scaling instead incorporated above; this can help in cases
        // where otherwise pow(two_pow_64, j) would overflow due to very
        // large coeff_mod_count and very large scale
        // res[i] = res_accum * inv_scale;
    }

    size_t tt = coeff_count;
    for (int i = 0; i < logn; i++)
    {
        size_t mm = (size_t)(1) << i;
        tt >>= 1;

        for (size_t j = 0; j < mm; j++)
        {
            size_t j1 = 2 * j * tt;
            size_t j2 = j1 + tt - 1;
            double complex s = self->roots_[mm + j];

            for (size_t k = j1; k < j2 + 1; k++)
            {
                double complex u = res[k];
                double complex v = res[k + tt] * s;
                res[k] = u + v;
                res[k + tt] = u - v;
                if (k == 0 || (k + tt) == 0) {
                    log_trace("k=%lu tt=%lu u=%lu v = %lu\n", k, tt, u, v);
                }
            }
        }
    }

    destination->clear(destination);
    destination->reserve(destination, self->slots_);
    for (size_t i = 0; i < self->slots_; i++)
    {
        destination->push_back(destination,
            creal(res[self->matrix_reps_index_map_[i]]));
    }

    hedge_free(plain_copy);
    hedge_free(temp);
    hedge_free(wide_tmp_dest);
    hedge_free(res);
}
static void decode(CKKSEncoder* self, Plaintext* plain,
            vector(double_complex)* destination)
{
    decode_internal(self, (Plaintext*)plain, destination);
}
        

CKKSEncoder* new_CKKSEncoder(hedge_context_t* context)
{
    CKKSEncoder* self = hedge_malloc(sizeof(CKKSEncoder));
    if (!self) {
        log_fatal("new_CKKSEncoder: unable to allocate CKKSEncoder!");
        return self;
    }

    self->encode = encode;
    self->encode_simple = encode_simple;
    self->decode = decode;
    self->slot_count = slot_count;


    self->context_ = context;
    // Verify parameters
    if (!self->context_)
    {
        invalid_argument("invalid context");
    }
    if (!self->context_->is_parms_set(self->context_))
    {
        invalid_argument("encryption parameters are not set correctly");
    }

    ctxdata_t* context_data = self->context_->first_ctxdata(self->context_);

    size_t coeff_count = context_data->parms->poly_modulus_degree(context_data->parms);
    log_trace("ckks: poly_modulus_degree => coeff_count = %zu\n", coeff_count);
    self->slots_ = coeff_count >> 1;
    int logn = get_power_of_two(coeff_count);

    self->matrix_reps_index_map_ = hedge_malloc(sizeof(uint64_t) * coeff_count);

    // Copy from the matrix to the value vectors
    uint64_t gen = 3;
    uint64_t pos = 1;
    uint64_t m = coeff_count << 1;
    for (size_t i = 0; i < self->slots_; i++)
    {
        // Position in normal bit order
        uint64_t index1 = (pos - 1) >> 1;
        uint64_t index2 = (m - pos - 1) >> 1;

        // Set the bit-reversed locations
        self->matrix_reps_index_map_[i] = reverse_bits(index1, logn);
        self->matrix_reps_index_map_[self->slots_ | i] = reverse_bits(index2, logn);

        // Next primitive root
        pos *= gen;
        pos &= (m - 1);
    }

    self->roots_ = hedge_malloc(sizeof(double complex) * coeff_count);
    self->inv_roots_ = hedge_malloc(sizeof(double complex) * coeff_count);

    double complex psi = cos((2 * PI_) / (double)m) + sin((2 * PI_) / (double)m)*I;
    
    for (size_t i = 0; i < coeff_count; i++) {
        self->roots_[i] = cpow(psi, (double)(reverse_bits(i, logn)));
        self->inv_roots_[i] = 1.0 / self->roots_[i];
    }

    return self;
}

void del_CKKSEncoder(CKKSEncoder* self)
{
    hedge_free(self->matrix_reps_index_map_);
    hedge_free(self->roots_);
    hedge_free(self->inv_roots_);
    hedge_free(self);
}
