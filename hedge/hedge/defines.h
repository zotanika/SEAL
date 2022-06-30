#ifndef __DEFINES_H__
#define __DEFINES_H__
// Bounds for bit-length of user-defined coefficient moduli
#define HEDGE_USER_MOD_BIT_COUNT_MAX 60
#define HEDGE_USER_MOD_BIT_COUNT_MIN 2

// Bounds for number of coefficient moduli
#define HEDGE_COEFF_MOD_COUNT_MAX 62
#define HEDGE_COEFF_MOD_COUNT_MIN 1

// Bounds for polynomial modulus degree
#define HEDGE_POLY_MOD_DEGREE_MAX 32768
#define HEDGE_POLY_MOD_DEGREE_MIN 2

// Bounds for the plaintext modulus
#define HEDGE_PLAIN_MOD_MIN HEDGE_USER_MOD_BIT_COUNT_MIN
#define HEDGE_PLAIN_MOD_MAX HEDGE_USER_MOD_BIT_COUNT_MAX

// Upper bound on the size of a ciphertext
#define HEDGE_CIPHERTEXT_SIZE_MIN 2
#define HEDGE_CIPHERTEXT_SIZE_MAX 16

// Detect compiler
#define HEDGE_COMPILER_MSVC 1
#define HEDGE_COMPILER_CLANG 2
#define HEDGE_COMPILER_GCC 3
#endif /* __DEFINES_H__ */