#ifndef __SMALLNTT_H__
#define __SMALLNTT_H__

#include "math/smallmodulus.h"

typedef struct SmallNTTTables_t SmallNTTTables;

struct SmallNTTTables_t {
    bool generated;
    uint64_t root;
    uint64_t inv_degree_modulo;
    int coeff_count_power;
    size_t coeff_count;

    uint64_t *root_powers;
    uint64_t *scaled_root_powers;
    uint64_t *inv_root_powers;
    uint64_t *scaled_inv_root_powers;
    uint64_t *inv_root_powers_div_two;
    uint64_t *scaled_inv_root_powers_div_two;
    Zmodulus *modulus;

    int (*init)(SmallNTTTables *self);
    void (*free)(SmallNTTTables *self);
    bool (*generate)(SmallNTTTables *self, int coeff_count_power, const Zmodulus *modulus);
    void (*reset)(SmallNTTTables *self);
    uint64_t (*get_from_root_powers)(SmallNTTTables *self, size_t index);
    uint64_t (*get_from_scaled_root_powers)(SmallNTTTables *self, size_t index);
    uint64_t (*get_from_inv_root_powers)(SmallNTTTables *self, size_t index);
    uint64_t (*get_from_scaled_inv_root_powers)(SmallNTTTables *self, size_t index);
    uint64_t (*get_from_inv_root_powers_div_two)(SmallNTTTables *self, size_t index);
    uint64_t (*get_from_scaled_inv_root_powers_div_two)(SmallNTTTables *self, size_t index);
    void (*ntt_powers_of_primitive_root)(SmallNTTTables *self, uint64_t root, uint64_t *dest);
    void (*ntt_scale_powers_of_primitive_root)(SmallNTTTables *self, const uint64_t* input, uint64_t *dest);
};

extern SmallNTTTables *new_SmallNTTTables(void);
extern void del_SmallNTTTables(SmallNTTTables *self);

extern void ntt_negacyclic_harvey_lazy(uint64_t *operand, const SmallNTTTables *tables);
extern void inverse_ntt_negacyclic_harvey_lazy(uint64_t *operand, const SmallNTTTables *small_ntt_tables);
extern void ntt_negacyclic_harvey(uint64_t *operand, const SmallNTTTables *tables);
extern void inverse_ntt_negacyclic_harvey(uint64_t *operand, const SmallNTTTables *tables);

#endif /* __SMALLNTT_H__ */