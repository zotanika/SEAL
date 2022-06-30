#ifndef __RNG_H__
#define __RNG_H__

#include "lib/types.h"
/**
 * Random Number Generator for HEDGE
 *
 * This is a basic pseudo random number generator
 * that generates a random number with a normal distribution.
 */

#define HEDGE_RNG_STATISTICS

#ifdef HEDGE_RNG_STATISTICS
#define HEDGE_RNG_STAT_COUNT 21
struct rng_stat {
    size_t count[HEDGE_RNG_STAT_COUNT];
    size_t samples;
};
#endif

typedef enum {
    RNG_UNIFORM,
    RNG_NORMAL
} rng_type;

struct rng {
    rng_type type;
    double mu;      /* mean */
    double sigma;   /* standard deviation */

    /* for internal use only */
	int x2_valid;
    double x1;
    double x2;
    double s;

#ifdef HEDGE_RNG_STATISTICS
    struct rng_stat statistics;
#endif
};

unsigned rng_get_seed(void);
void rng_init(struct rng* rng, unsigned seed, double mean, double stddev);
void rng_init_normal_default(struct rng* rng);
void rng_init_uniform(struct rng* rng, unsigned seed, double min, double max);
void rng_init_uniform_default(struct rng* rng);
void rng_reset(struct rng* rng, unsigned seed);
double rng_get(struct rng* rng);
int rng_get_rounded_int(struct rng* rng);
double rng_get_maxdev(struct rng*, double max_deviation);
#ifdef HEDGE_RNG_STATISTICS
int rng_print_statistics(struct rng *rng);
#endif

#endif /* __RNG_H__ */
