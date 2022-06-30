#ifdef __cplusplus
extern "C" {
#endif

#include "context.h"
#include "math/polycore.h"
#include "math/uintarith.h"
#include "math/uintarithsmallmod.h"
#include "math/numth.h"
#include "math/baseconverter.h"
#include "math/modulus.h"
#include "lib/common.h"

DECLARE_UNORDERED_MAP_BODY(parms_id_type, ctxdata_ptr);

static void init_epq(struct encrypt_parm_qualifiers *self)
{
    self->parameters_set = false;
    self->using_fft = false;
    self->using_ntt = false;
    self->using_batching = false;
    self->using_fast_plain_lift = false;
    self->using_descending_modulus_chain = false;
    self->sec_level = sec_level_type_none;
}

struct encrypt_parm_qualifiers *new_epq(void)
{
    struct encrypt_parm_qualifiers *obj = hedge_zalloc(sizeof(struct encrypt_parm_qualifiers));
    if (!obj)
        return NULL;

    *obj = (struct encrypt_parm_qualifiers) {
        .init = init_epq,
        .free = NULL
    };
    obj->init(obj);
    return obj;
}

void del_epq(struct encrypt_parm_qualifiers *self)
{
    if (self->free)
        self->free(self);
    hedge_free(self);
}

struct encrypt_parm_qualifiers* new_qualifiers(void)
{
    struct encrypt_parm_qualifiers* obj = hedge_malloc(sizeof(struct encrypt_parm_qualifiers));
    // TODO : 
    
    return obj;
}

static void init_ctxdata(struct ctxdata *self, const struct encrypt_parameters *parms)
{
    self->parms = (struct encrypt_parameters *)parms;
    self->total_coeff_modulus_bit_count = 0;
    self->plain_upper_half_threshold = 0;
    self->chain_index = 0;
    self->qualifiers = new_qualifiers();
}

/* TODO? */
static void copy_ctxdata_from(struct ctxdata *self, struct ctxdata *src) {}

static void move_ctxdata_from(struct ctxdata *self, struct ctxdata *src)
{
    self->parms = src->parms;
    self->qualifiers = src->qualifiers;
    self->base_converter = src->base_converter;
    self->small_ntt_tables = src->small_ntt_tables;
    self->plain_ntt_tables = src->plain_ntt_tables;

    self->total_coeff_modulus_bit_count = src->total_coeff_modulus_bit_count;
    self->plain_upper_half_threshold = src->plain_upper_half_threshold;
    self->chain_index = src->chain_index;

    self->total_coeff_modulus = src->total_coeff_modulus;
    self->coeff_div_plain_modulus = src->coeff_div_plain_modulus;
    self->plain_upper_half_increment = src->plain_upper_half_increment;
    self->upper_half_threshold = src->upper_half_threshold;
    self->upper_half_increment = src->upper_half_increment;
    self->prev_context_data = src->prev_context_data;
    self->next_context_data = src->next_context_data;
}

static void free_ctxdata(struct ctxdata *self) 
{
    int i;
    int coeff_mod_count = self->parms->coeff_modulus(self->parms)->size;

    log_trace("free_ctxdata: coeff_mod_count=%zu\n", coeff_mod_count);
    // TODO: del_SmallNTTTables
    for (i = 0; i < coeff_mod_count; i++) {
        SmallNTTTables * tbl = self->small_ntt_tables[i];
        log_trace("tbl=%p\n", tbl);
        tbl->reset(tbl);
        del_SmallNTTTables(tbl);
    }
    hedge_free(self->small_ntt_tables);

    del_encrypt_parameters(self->parms);
    // TODO need to delete base converter
    del_baseconverter(self->base_converter);

    hedge_free(self->qualifiers);
    hedge_free(self->total_coeff_modulus);
    hedge_free(self->coeff_div_plain_modulus);
    hedge_free(self->plain_upper_half_increment);
    hedge_free(self->upper_half_threshold);
    hedge_free(self->upper_half_increment);
};

struct ctxdata *new_ctxdata(const struct encrypt_parameters *parms)
{
    struct ctxdata *obj = hedge_zalloc(sizeof(struct ctxdata));
    if (!obj)
        return NULL;

    *obj = (struct ctxdata) {
        .init = init_ctxdata,
        .free = free_ctxdata,
        .copy = copy_ctxdata_from,
        .move = move_ctxdata_from,
    };

    obj->init(obj, parms);
    return obj;
}

void del_ctxdata(struct ctxdata *self)
{
    if (self->free)
        self->free(self);
    hedge_free(self);
}

static void init_common(struct hedge_context* self)
{
    list_init(&self->object_list);
    self->context_data_map = new_unordered_map(parms_id_type, ctxdata_ptr, 3);
}

static void init_hcontext(struct hedge_context *self)
{
}

/**
 * Creates an instance of hedge_context_t, and performs several pre-computations
 * on the given encrypt_parameters.
 * 
 * @param[in] parms The encryption parameters
 * @param[in] expand_mod_chain Determines whether the modulus switching chain
 * should be created
 * @param[in] sec_level Determines whether a specific security level should be
 * enforced according to HomomorphicEncryption.org security standard
 * @throws std::invalid_argument if pool is uninitialized
 */
struct ctxdata* key_parms_id_context;
void print_key_parms_id_context(int line)
{
    static int cnt = 0;
    log_trace(YELLOW_BOLD "================\n" RESET);
    log_trace("@@ TRACE [%d:%d]@@ : key_parms_id_context=%p\n", cnt++, line, key_parms_id_context);
    print_array_trace(key_parms_id_context->parms->parms_id_.data, 4);
}
static void init_hcontext_with_parms(
        struct hedge_context *self,
        struct encrypt_parameters *parms,
        bool expand_mod_chain,
        sec_level_type sec_level)
{
    size_t parms_count;
    parms_id_type next_parms_id, prev_parms_id;
    struct ctxdata *context_data_ptr;

    //if (!parms->random_generator()) {
    //    parms->set_random_generator(UniformRandomGeneratorFactory::default_factory());
    //}

    /* Validate parameters and add new ctxdata_t to the map
     * Note that this happens even if parameters are not valid */
    context_data_ptr = self->validate(self, parms);

    // DEBUG {
    key_parms_id_context = context_data_ptr;
    log_trace("*** value for key_parms_id should be %p\n", key_parms_id_context);
    print_key_parms_id_context(__LINE__);
    //}
    
    self->context_data_map->emplace(self->context_data_map,
            unordered_map_entry(parms_id_type, parms->parms_id_, ctxdata_ptr, context_data_ptr));
    self->key_parms_id = parms->parms_id_;
    
    /* Then create first_parms_id_ if the parameters are valid and there is
     * more than one modulus in coeff_modulus. This is equivalent to expanding
     * the chain by one step. Otherwise, we set first_parms_id_ to equal
     * key_parms_id_. */
    struct ctxdata* cdata = self->context_data_map->value_at(self->context_data_map, self->key_parms_id);
    if (!cdata->qualifiers->parameters_set ||
        parms->coeff_modulus_->size == 1) {
        self->first_parms_id = self->key_parms_id;
        print_key_parms_id_context(__LINE__);
    } else {
        next_parms_id = self->create_next_ctxdata(self, &self->key_parms_id);
        self->first_parms_id =
            EQUALS(&next_parms_id, &parms_id_zero) ? self->key_parms_id : next_parms_id;
    }

    /* Set last_parms_id_ to point to first_parms_id_ */
    self->last_parms_id = self->first_parms_id;
    /* Check if keyswitching is available */
    self->using_keyswitching = !EQUALS(&self->first_parms_id, &self->key_parms_id);

    print_key_parms_id_context(__LINE__);
    /* If modulus switching chain is to be created, compute the remaining
     * parameter sets as long as they are valid to use (parameters_set == true) */
    cdata = self->context_data_map->value_at(self->context_data_map, self->first_parms_id);
    if (expand_mod_chain && cdata->qualifiers->parameters_set) {
        prev_parms_id = self->first_parms_id;
        cdata = self->context_data_map->value_at(self->context_data_map, prev_parms_id);
        while (cdata->parms->coeff_modulus_->size > 1) {
            next_parms_id = self->create_next_ctxdata(self, &prev_parms_id);
            if (EQUALS(&next_parms_id, &parms_id_zero))
                break;
            prev_parms_id = next_parms_id;
            self->last_parms_id = next_parms_id;
            cdata = self->context_data_map->value_at(self->context_data_map, prev_parms_id);
        }
    }

    /* Set the chain_index for each context_data */
    parms_count = self->context_data_map->size;
    context_data_ptr = self->context_data_map->value_at(self->context_data_map, self->key_parms_id);
    while (context_data_ptr) {
        /* We need to remove constness first to modify this */
        context_data_ptr->chain_index = --parms_count;
        context_data_ptr = context_data_ptr->next_context_data;
    }
    print_key_parms_id_context(__LINE__);
}

static void free_hcontext(struct hedge_context *self)
{
    struct ctxdata* curr;
    struct ctxdata* next;
    curr = self->context_data_map->value_at(self->context_data_map, self->key_parms_id);
    while (curr) {
        next = curr->next_context_data;
        log_trace("free_hcontext %p\n", self);
        del_ctxdata(curr);
        curr = next;
    }
    self->context_data_map->delete(self->context_data_map);
}

/**
 * Returns hedge_context corresponding to encryption parameters with a given
 * parms_id. If parameters with the given parms_id are not found then the
 * function returns nullptr.
 * 
 * @param[in] parms_id The parms_id of the encryption parameters
 */
static struct ctxdata *get_hcontext_ctxdata(
        struct hedge_context *self,
        parms_id_type* parms_id)
{
    return self->context_data_map->value_at(self->context_data_map, *parms_id);
}

/**
 * Returns hedge_context corresponding to encryption parameters that are
 * used for keys.
 */
static struct ctxdata *key_hcontext_ctxdata(struct hedge_context *self)
{
    return self->context_data_map->value_at(self->context_data_map, self->key_parms_id);
}

/**
 * Returns hedge_context corresponding to the first encryption parameters
 * that are used for data.
 */
static struct ctxdata *first_hcontext_ctxdata(struct hedge_context *self)
{
    return self->context_data_map->value_at(self->context_data_map, self->first_parms_id);
}

/**
 * Returns hedge_context corresponding to the last encryption parameters
 * that are used for data.
 */
static struct ctxdata *last_hcontext_ctxdata(struct hedge_context *self)
{
    return self->context_data_map->value_at(self->context_data_map, self->last_parms_id);
}

/**
 * Returns whether the encryption parameters are valid.
 */
static bool is_hcontext_parms_set(struct hedge_context *self)
{
    if (self->first_ctxdata) {
        struct ctxdata *ctxdata = self->first_ctxdata(self);
        return ctxdata ? ctxdata->qualifiers->parameters_set : false;
    } else {
        return false;
    }
}

/**
 * Creates an instance of hedge_context and performs several pre-computations
 * on the given encrypt_parameters.
 * 
 * @param[in] parms The encryption parameters
 * @param[in] expand_mod_chain Determines whether the modulus switching chain
 * should be created
 * @param[in] sec_level Determines whether a specific security level should be
 * enforced according to HomomorphicEncryption.org security standard
 */
static struct ctxdata* create_data_for_hcontext(
        struct hedge_context *self,
        const struct encrypt_parameters *parms,
        bool expand_mod_chain,
        sec_level_type sec_level)
{
    self->sec_level = (sec_level == sec_level_type_none) ? sec_level_type_tc128 : sec_level;
    self->data = new_ctxdata(parms);
    return self->data;
}

/**
 * Create the next context_data by dropping the last element from coeff_modulus.
 * If the new encryption parameters are not valid, returns parms_id_zero.
 * Otherwise, returns the parms_id of the next parameter and appends the next
 * context_data to the chain.
 */
static parms_id_type create_hcontext_next_ctxdata(
        struct hedge_context *self,
        const parms_id_type *prev_parms_id)
{
    /* Create the next set of parameters by removing last modulus */
    struct ctxdata* cdata = self->context_data_map->value_at(self->context_data_map, *prev_parms_id);
    struct encrypt_parameters *next_parms = dup_encrypt_parameters(cdata->parms);
    print_key_parms_id_context(__LINE__);
    vector(Zmodulus) *next_coeff_modulus = next_parms->coeff_modulus_;
    next_coeff_modulus->pop_back(next_coeff_modulus);
    print_key_parms_id_context(__LINE__);
    next_parms->set_coeff_modulus(next_parms, next_coeff_modulus);
    parms_id_type next_parms_id = next_parms->parms_id_;
    log_trace("** create_next_context_data: next_parms size : %zu\n", 
        next_parms->coeff_modulus_->size);

    /* Validate next parameters and create next context_data */
    struct ctxdata *next_context_data = self->validate(self, next_parms);
    if (!next_context_data->qualifiers->parameters_set)
        return parms_id_zero;

    /* Add them to the context_data_map */
    self->context_data_map->emplace(self->context_data_map, 
        unordered_map_entry(parms_id_type, next_parms_id, ctxdata_ptr, next_context_data));

    /* Add pointer to next context_data to the previous one (linked list)
     * Add pointer to previous context_data to the next one (doubly linked list)
     * We need to remove constness first to modify this */
    cdata = self->context_data_map->value_at(self->context_data_map, *prev_parms_id);
    cdata->next_context_data =
        self->context_data_map->value_at(self->context_data_map, next_parms_id);
    cdata = self->context_data_map->value_at(self->context_data_map, next_parms_id);
    cdata->prev_context_data =
        self->context_data_map->value_at(self->context_data_map, *prev_parms_id);

    return next_parms_id;
}

static struct ctxdata *validate_data_for_hcontext(
        struct hedge_context *self,
        struct encrypt_parameters *parms)
{
    vector(Zmodulus)* coeff_modulus;
    Zmodulus* plain_modulus;
    size_t coeff_mod_count;
    size_t i, j;
    size_t poly_modulus_degree;
    uint64_t *temp;
    int coeff_count_power;
    struct ctxdata *context_data = new_ctxdata(parms);
    log_trace("validate_data_for_hcontext: validate: %zu\n", context_data->parms->coeff_modulus_->size);
    log_trace("validate_data_for_hcontext: addr of coeff_modulus_ : %p\n", &context_data->parms->coeff_modulus_);
    
    if (!context_data)
        return NULL;

    log_trace("validate_data_for_hcontext: hedge_context=%p\n", self);
    context_data->qualifiers->parameters_set = true;

    coeff_modulus = parms->coeff_modulus(parms);
    plain_modulus = parms->plain_modulus(parms);
    coeff_mod_count = coeff_modulus->size;

    /* The number of coeff moduli is restricted to 62 for lazy reductions
     * in baseconverter.cpp to work */
    if (coeff_modulus->size > HEDGE_COEFF_MOD_COUNT_MAX ||
        coeff_modulus->size < HEDGE_COEFF_MOD_COUNT_MIN) {
        context_data->qualifiers->parameters_set = false;
        return context_data;
    }

    for (i = 0; i < coeff_mod_count; i++) {
        /* Check coefficient moduli bounds */
        if (coeff_modulus->at(coeff_modulus, i)->value >> HEDGE_USER_MOD_BIT_COUNT_MAX ||
            !(coeff_modulus->at(coeff_modulus, i)->value >> (HEDGE_USER_MOD_BIT_COUNT_MIN - 1))) {
            context_data->qualifiers->parameters_set = false;
            return context_data;
        }
        /* Check that all coeff moduli are pairwise relatively prime */
        for (j = 0; j < i; j++) {
            if (gcd(coeff_modulus->at(coeff_modulus, i)->value, coeff_modulus->at(coeff_modulus, j)->value) > 1) {
                context_data->qualifiers->parameters_set = false;
                return context_data;
            }
        }
    }

    /* Compute the product of all coeff moduli */
    context_data->total_coeff_modulus = allocate_uint(coeff_mod_count);
    temp = allocate_uint(coeff_mod_count);

    set_uint(1, coeff_mod_count, context_data->total_coeff_modulus);
    for (i = 0; i < coeff_mod_count; i++) {
        multiply_uint_uint64(context_data->total_coeff_modulus,
                coeff_mod_count, coeff_modulus->at(coeff_modulus, i)->value,
                coeff_mod_count, temp);
        set_uint_uint(temp, coeff_mod_count, context_data->total_coeff_modulus);
    }
    hedge_free(temp);

    context_data->total_coeff_modulus_bit_count =
        get_significant_bit_count_uint(
                context_data->total_coeff_modulus, coeff_mod_count);

    /* Check polynomial modulus degree and create poly_modulus */
    poly_modulus_degree = parms->poly_modulus_degree_;
    coeff_count_power = get_power_of_two(poly_modulus_degree);
    if (poly_modulus_degree < HEDGE_POLY_MOD_DEGREE_MIN ||
        poly_modulus_degree > HEDGE_POLY_MOD_DEGREE_MAX ||
        coeff_count_power < 0) {
        context_data->qualifiers->parameters_set = false;
        return context_data;
    }

    /* Quick sanity check */
    if (!product_fits_in(coeff_mod_count, poly_modulus_degree)) {
        logic_error("invalid parameters");
    }
    

    /* Polynomial modulus X^(2^k) + 1 is guaranteed at this point */
    context_data->qualifiers->using_fft = true;
    /* Assume parameters satisfy desired security level */
    context_data->qualifiers->sec_level = self->sec_level;

    /* Check if the parameters are secure according to HomomorphicEncryption.org
     * security standard */
    if (context_data->total_coeff_modulus_bit_count > 
            CoeffModulus_MaxBitCount(poly_modulus_degree, self->sec_level)) {
        /* Not secure according to HomomorphicEncryption.org security standard */
        context_data->qualifiers->sec_level = sec_level_type_none;
        if (self->sec_level != sec_level_type_none) {
            context_data->qualifiers->parameters_set = false;
            return context_data;
        }
    }

    /* Can we use NTT with coeff_modulus? */
    context_data->qualifiers->using_ntt = true;
    context_data->small_ntt_tables = hedge_zalloc(sizeof(SmallNTTTables*) * coeff_mod_count);
    for (i = 0; i < coeff_mod_count; i++) {
        SmallNTTTables * tbl = context_data->small_ntt_tables[i] = new_SmallNTTTables();
        if (!tbl->generate(tbl, coeff_count_power, coeff_modulus->at(coeff_modulus, i))) {
            context_data->qualifiers->using_ntt = false;
            context_data->qualifiers->parameters_set = false;
            return context_data;
        }
    }

    if (parms->scheme_ == CKKS) {
        if (plain_modulus->value != 0) {
            context_data->qualifiers->parameters_set = false;
            return context_data;
        }

        /* When using CKKS batching (BatchEncoder) is always enabled */
        context_data->qualifiers->using_batching = true;
        /* Cannot use fast_plain_lift for CKKS since the plaintext coefficients
         * can easily be larger than coefficient moduli */
        context_data->qualifiers->using_fast_plain_lift = false;
        /* Calculate 2^64 / 2 (most negative plaintext coefficient value) */
        context_data->plain_upper_half_threshold = (uint64_t)(1) << 63;

        /* Calculate plain_upper_half_increment = 2^64 mod coeff_modulus for CKKS plaintexts */
        context_data->plain_upper_half_increment = allocate_uint(coeff_mod_count);
        for (i = 0; i < coeff_mod_count; i++) {
            uint64_t tmp = ((uint64_t)(1) << 63) % coeff_modulus->at(coeff_modulus, i)->value;
            context_data->plain_upper_half_increment[i] = multiply_uint_uint_smallmod(
                    tmp, sub_safe(coeff_modulus->at(coeff_modulus, i)->value, (uint64_t)(2)), coeff_modulus->at(coeff_modulus, i));
        }

        /* Compute the upper_half_threshold for this modulus. */
        context_data->upper_half_threshold = allocate_uint(coeff_mod_count);
        increment_uint(context_data->total_coeff_modulus,
                coeff_mod_count, context_data->upper_half_threshold);
        right_shift_uint(context_data->upper_half_threshold, 1,
                coeff_mod_count, context_data->upper_half_threshold);
    } else {
        // throw invalid_argument("unsupported scheme");
    }

    /* Create BaseConverter */
    context_data->base_converter = new_baseconverter();
    context_data->base_converter->generate(context_data->base_converter, coeff_modulus, poly_modulus_degree, plain_modulus);
    if (!context_data->base_converter->generated) {
        context_data->qualifiers->parameters_set = false;
        return context_data;
    }

    /* Check whether the coefficient modulus consists of a set of primes that
     * are in decreasing order */
    context_data->qualifiers->using_descending_modulus_chain = true;
    for (i = 0; i < coeff_mod_count - 1; i++) {
        context_data->qualifiers->using_descending_modulus_chain
            &= (coeff_modulus->at(coeff_modulus, i)->value > 
                coeff_modulus->at(coeff_modulus, i + 1)->value);
    }

    return context_data;
}

struct hedge_context *new_hcontext(
    encrypt_parameters* parms,
    bool expand_mod_chain,
    sec_level_type sec_level)
{
    struct hedge_context *obj = hedge_zalloc(sizeof(struct hedge_context));
    if (!obj) {
        log_fatal("Unable to allocate hedge_context");
        return NULL;
    }

    *obj = (struct hedge_context) {
        .init = init_hcontext,
        .init_with_parms = init_hcontext_with_parms,
        .free = free_hcontext,
        .create = create_data_for_hcontext,
        .create_next_ctxdata = create_hcontext_next_ctxdata,
        .validate = validate_data_for_hcontext,
        .get_ctxdata = get_hcontext_ctxdata,
        .key_ctxdata = key_hcontext_ctxdata,
        .first_ctxdata = first_hcontext_ctxdata,
        .last_ctxdata = last_hcontext_ctxdata,
        .is_parms_set = is_hcontext_parms_set
    };

    init_common(obj);

    parms = dup_encrypt_parameters(parms);

    if (parms) {
        if (sec_level == 0)
            obj->sec_level = sec_level_type_tc128;
        obj->init_with_parms(obj, parms, expand_mod_chain, sec_level);
    } else {
        obj->init(obj);
    }

    return obj;
}

void del_hcontext(struct hedge_context *self)
{
    if (self->free)
        self->free(self);
    struct node *entry;
    iterate_list(self->object_list, entry) {
        hedge_free(entry->data);
    }
    del_list(&self->object_list);
    hedge_free(self);
}

#ifdef __cplusplus
}
#endif