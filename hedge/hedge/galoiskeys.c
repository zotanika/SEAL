#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include "galoiskeys.h"

/**
 * Returns the index of a Galois key in the backing KSwitchKeys instance that
 * corresponds to the given Galois element, assuming that it exists in the
 * backing KSwitchKeys.
 * 
 * @param[in] galois_elt The Galois element
 * @throws std::invalid_argument if galois_elt is not valid
 */
static size_t get_galoiskey_index(uint64_t galois_elt)
{
    assert(galois_elt & 1);
    return (size_t)((galois_elt - 1) >> 1);
}

/**
 * Returns whether a Galois key corresponding to a given Galois element exists.
 * 
 * @param[in] galois_elt The Galois element
 * @throws std::invalid_argument if galois_elt is not valid
 */
static bool has_galoiskey(struct galois_keys *self, uint64_t galois_elt)
{
    size_t index = self->get_index(galois_elt);
    vector(vector_publickey) *ikeys = self->parent->keys;
    return (ikeys->size > index) && (ikeys->at(ikeys, index)->size > 0);
}

/**
 * Returns a const reference to a Galois key. The returned Galois key corresponds
 * to the given Galois element.
 * 
 * @param[in] galois_elt The Galois element
 * @throws std::invalid_argument if the key corresponding to galois_elt does not exist
 */
static struct galois_keys *galoiskey(struct galois_keys *self, uint64_t galois_elt)
{
    size_t index = self->get_index(galois_elt);
    vector(vector_publickey) *ikeys = self->parent->keys;
    return (struct galois_keys *)ikeys->at(ikeys, index);
}

struct galois_keys *new_galoiskeys(void)
{
    struct galois_keys *inst = hedge_zalloc(sizeof(struct galois_keys));
    if (!inst)
        return NULL;
    
    *inst = (struct galois_keys) {
        .get_index = get_galoiskey_index,
        .has_key = has_galoiskey,
        .key = galoiskey
    };

    inst->parent = new_kswitchkeys();
    inst->parent->init(inst->parent);
    return inst;
}

void del_galoiskeys(struct galois_keys *self)
{
    del_kswitchkeys(self->parent);
    hedge_free(self);
}

#ifdef __cplusplus
}
#endif