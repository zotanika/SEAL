#ifndef __BASECONVERTER_H__
#define __BASECONVERTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lib/vector.h"
#include "math/smallmodulus.h"
#include "math/smallntt.h"


typedef struct base_converter base_converter_t;
typedef struct base_converter BaseConverter;

struct base_converter {
    bool generated;
    size_t coeff_count;
    size_t coeff_base_mod_count;
    size_t aux_base_mod_count;
    size_t bsk_base_mod_count;
    size_t plain_gamma_count;
    Zmodulus **coeff_base_array;
    Zmodulus **aux_base_array;
    Zmodulus **bsk_base_array;
    Zmodulus **plain_gamma_array;
    uint64_t *coeff_products_array;
    uint64_t **coeff_base_products_mod_aux_bsk_array;
    uint64_t *inv_coeff_base_products_mod_coeff_array;
    uint64_t *coeff_base_products_mod_mtilde_array;
    uint64_t *mtilde_inv_coeff_base_products_mod_coeff_array;
    uint64_t *inv_coeff_products_all_mod_aux_bsk_array;
    uint64_t **aux_base_products_mod_coeff_array;
    uint64_t *inv_aux_base_products_mod_aux_array;
    uint64_t *aux_base_products_mod_msk_array;
    uint64_t inv_coeff_products_mod_mtilde;
    uint64_t inv_aux_products_mod_msk;
    uint64_t inv_gamma_mod_plain;
    uint64_t *aux_products_all_mod_coeff_array;
    uint64_t *inv_mtilde_mod_bsk_array;
    uint64_t *coeff_products_all_mod_bsk_array;
    uint64_t **coeff_products_mod_plain_gamma_array;
    uint64_t *neg_inv_coeff_products_all_mod_plain_gamma_array;
    uint64_t *plain_gamma_product_mod_coeff_array;
    SmallNTTTables **bsk_small_ntt_tables;
    uint64_t *inv_last_coeff_mod_array;
    Zmodulus *m_tilde;
    Zmodulus *m_sk;
    Zmodulus *small_plain_mod;
    Zmodulus *gamma;
    void (*init)(struct base_converter *self);
    void (*init_with_parms)(struct base_converter *self, vector(Zmodulus) *coeff_base,
            size_t coeff_count, const Zmodulus *small_plain_mod);
    void (*assign)(struct base_converter *self, struct base_converter *src);
    void (*free)(struct base_converter *self);
    void (*generate)(struct base_converter *self, vector(Zmodulus) *coeff_base,
            size_t coeff_count, const Zmodulus *small_plain_mod);
    void (*floor_last_coeff_modulus_inplace)(struct base_converter *self, uint64_t *rns_poly);
    void (*floor_last_coeff_modulus_ntt_inplace)(struct base_converter *self,
            uint64_t *rns_poly, const SmallNTTTables **rns_ntt_tables);
    void (*fastbconv)(struct base_converter *self, const uint64_t *input, uint64_t *dest);
    void (*fastbconv_sk)(struct base_converter *self, const uint64_t *input, uint64_t *dest);
    void (*mont_rq)(struct base_converter *self, const uint64_t *input, uint64_t *dest);
    void (*fast_floor)(struct base_converter *self, const uint64_t *input, uint64_t *dest);
    void (*fastbconv_mtilde)(struct base_converter *self, const uint64_t *input, uint64_t *dest);
    void (*fastbconv_plain_gamma)(struct base_converter *self, const uint64_t *input, uint64_t *dest);    
    void (*reset)(struct base_converter *self);
};

extern struct base_converter *new_baseconverter(void);
extern void del_baseconverter(struct base_converter *inst);

#ifdef __cplusplus
}
#endif

#endif /* __BASECONVERTER_H__ */