#include "relinkeys.h"

/**
 * Returns the index of a relinearization key in the backing KSwitchKeys
 * instance that corresponds to the given secret key power, assuming that
 * it exists in the backing KSwitchKeys.
 * 
 * @param[in] key_power The power of the secret key
 * @throws std::invalid_argument if key_power is less than 2
 */
static size_t get_index(relinkeys_t *self, size_t key_power)
{
    if (key_power < 2) {
        /* invalid key_power */
        return 0;
    }
    return key_power - 2;
}

/**
 * Returns whether a relinearization key corresponding to a given power of
 * the secret key exists.
 * 
 * @param[in] key_power The power of the secret key
 * @throws std::invalid_argument if key_power is less than 2
 */
static bool has_key(relinkeys_t *self, size_t key_power)
{
    size_t index = self->get_index(self, key_power);
    vector(vector_publickey) *ikeys = self->parent->keys;
    return (ikeys->size > index) && (ikeys->at(ikeys, index)->size > 0);
}

/**
 * Returns a const reference to a relinearization key. The returned
 * relinearization key corresponds to the given power of the secret key.
 * 
 * @param[in] key_power The power of the secret key
 * @throws std::invalid_argument if the key corresponding to key_power does not exist
 */
static relinkeys_t *key(relinkeys_t *self, size_t key_power)
{
    size_t index = self->get_index(self, key_power);
    vector(vector_publickey) *ikeys = self->parent->keys;
    return (relinkeys_t *)ikeys->at(ikeys, index);
}

relinkeys_t *new_relinkeys(void)
{
    relinkeys_t *inst = hedge_zalloc(sizeof(relinkeys_t));
    if (!inst)
        return NULL;

    *inst = (relinkeys_t) {
        .get_index = get_index,
        .has_key = has_key,
        .key = key
    };
    inst->parent = new_kswitchkeys();
    return inst;
}

void del_relinkeys(relinkeys_t *self)
{
    del_kswitchkeys(self->parent);
    hedge_free(self);
}