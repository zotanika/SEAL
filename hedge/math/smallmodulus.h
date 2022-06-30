#ifndef __SMALLMODULUS_H__
#define __SMALLMODULUS_H__

#include "lib/vector.h"
#include "lib/bytestream.h"
#include "lib/unordered_map.h"
typedef struct Zmodulus Zmodulus;

DECLARE_VECTOR_TYPE(Zmodulus);

DECLARE_UNORDERED_MAP_TYPE(int, vector_Zmodulus);

struct Zmodulus {
    uint64_t value;
    uint64_t const_ratio[3];
    size_t uint64_count;
    int bit_count;
    bool is_prime;
};

extern Zmodulus* new_Zmodulus(uint64_t val);
extern Zmodulus* dup_Zmodulus(Zmodulus* src);
extern void del_Zmodulus(Zmodulus *p);
extern Zmodulus* set_zmodulus_with(Zmodulus* container, uint64_t value);

#endif /* __SMALLMODULUS_H__ */
