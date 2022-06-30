/**
 * Random Number Generator for HEDGE
 * 
 * This is a basic pseudo random number generator 
 * that generates a random number with a normal distribution.
 */
 
#include <math.h>
#include <tgmath.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include "rng.h"
#include "lib/debug.h"

#define ARRAY_SIZE(arry) (int)(sizeof(arry)/sizeof(arry[0]))

#if defined(HEDGE_USE_TEEGRIS)
#define RAND_MAX (0x7fffffff)
#endif

unsigned long __attribute((weak)) hedge_generate_random_seed(void)
{
    return (unsigned)(time((time_t*)0) + clock());
}

unsigned rng_get_seed(void)
{
    return hedge_generate_random_seed(); 
}
/**
 * Parameters :
 *  struct rng* : address to a struct rng to be used throughout the
 *                 random number generation
 *  seed : a seed to be used for the random number generator
 *  mean : mean value
 * stddev : standard deviation
 */
void rng_init(struct rng* rng, unsigned seed, double mean, double stddev)
{
    srand(seed);
    rng->type = RNG_NORMAL;
    rng->mu = mean;
    rng->sigma = stddev;
    rng->x2_valid = 0;
    rng->x1 = 0;
    rng->x2 = 0;
    rng->s = 0;
#ifdef HEDGE_RNG_STATISTICS
    memset(&rng->statistics, 0, sizeof(rng->statistics));
#endif
}

void rng_init_normal_default(struct rng* rng)
{
    rng_init(rng, rng_get_seed(), 0, INT_MAX);
}

void rng_init_normal(struct rng *rng, double mean, double stddev)
{
    rng_init(rng, rng_get_seed(), mean, stddev);
}

void rng_init_uniform(struct rng* rng, unsigned seed, double min, double max)
{
    srand(seed);
    rng->type = RNG_UNIFORM;
    rng->x1 = min;
    rng->x2 = max;
    rng->mu = (min + max) / 2.0;
#ifdef HEDGE_RNG_STATISTICS
    memset(&rng->statistics, 0, sizeof(rng->statistics));
#endif
}

void rng_init_uniform_default(struct rng* rng)
{
    rng_init_uniform(rng, rng_get_seed(), 0, INT_MAX);
}

/**
 * Resets all the statistics
 */
void rng_reset(struct rng* rng, unsigned seed)
{
    if (rng->type == RNG_NORMAL)
        rng_init(rng, seed, rng->mu, rng->sigma);
}


static double rng_return(struct rng* rng, double rn)
{
#ifdef HEDGE_RNG_STATISTICS
    int size = ARRAY_SIZE(rng->statistics.count);
    int idx = round(rn - rng->mu) + (int)(size / 2);
    if (idx >= 0 && idx < size)
        rng->statistics.count[idx]++;
#endif
    return rn;
}

/**
 * Uses Marsaglia polar method for generating a normally distributed 
 * random number
 */
static double rng_normal(struct rng* rng)
{
#ifdef HEDGE_RNG_STATISTICS
    rng->statistics.samples++;
#endif
    if(rng->x2_valid) {
        rng->x2_valid = 0;
        return rng_return(rng, rng->mu + rng->sigma * rng->x2);
    }

    do {
        rng->x1 = (rand() / ((double) RAND_MAX)) * 2.0 - 1.0;
        rng->x2 = (rand() / ((double) RAND_MAX)) * 2.0 - 1.0;
        rng->s = rng->x1 * rng->x1 + rng->x2 * rng->x2;
    } while( (rng->s >= 1.0) || (rng->s == 0.0) );

    rng->s = sqrt(-2.0 * log(rng->s) / rng->s);
    rng->x1 = rng->x1 * rng->s;
    rng->x2 = rng->x2 * rng->s;
    rng->x2_valid = 1;
    
    return rng_return(rng, rng->mu + rng->sigma * rng->x1);
}

static double rng_uniform(struct rng* rng)
{
    double rn = (double)rand() / RAND_MAX;
#ifdef HEDGE_RNG_STATISTICS
    rng->statistics.samples++;
#endif
    
    rn = rn * (rng->x2 - rng->x1) + rng->x1;
    return rng_return(rng, rn);
}

double rng_get(struct rng* rng)
{
    double ret;
    switch (rng->type) {
        case RNG_NORMAL:
            ret = rng_normal(rng);
            break;
        case RNG_UNIFORM:
            ret = rng_uniform(rng);
            break;
        default:
            ret = rng_normal(rng);
            break;
    }
    return ret;
}

int rng_get_rounded_int(struct rng* rng)
{
    return (int)round(rng_get(rng));
}

double rng_get_maxdev(struct rng* rng, double max_deviation)
{
    while (1) {
        double rn = rng_get(rng);
        double deviation = fabs(rn - rng->mu);
        if (deviation <= max_deviation) {
            return rn;
        }
    }
}

int rng_print_statistics(struct rng *rng)
{
    size_t inc = rng->statistics.samples / 100;
    size_t size = ARRAY_SIZE(rng->statistics.count);
    if (inc <= 0) inc = 1;
    for (size_t i = 0; i < size; i++) {
        printf("%10ld | ", i + (int)rng->mu - size/2);
        for (int n = 0; n < rng->statistics.count[i]; n += inc)
            printf("=");
        printf("> (%ld)\n", rng->statistics.count[i]);
    }
    return 0;
}

static void rng_test(double mean, double stddev, size_t num_samples)
{
    struct rng rng;
    double r;
    time_t t;

    printf("----------------------------------------------------------\n");
    printf(" HEDGE RNG UNIT TEST:\n");
    printf(" %22s: %ld\n", "samples", num_samples);
    printf(" %22s: %0.2lf\n", "mean", mean);
    printf(" %22s: %0.2lf\n", "standard deviation", stddev);
    printf("----------------------------------------------------------\n\n");

    rng_init(&rng, (unsigned)time(&t), mean, stddev);
    for (int i = 0; i < num_samples; i++) {
        r = rng_get(&rng);
    }
    rng_print_statistics(&rng);
    printf("----------------------------------------------------------\n\n");
}


static void rng_test_uniform(double min, double max, size_t num_samples)
{
    struct rng rng;
    double r;
    time_t t;

    printf("----------------------------------------------------------\n");
    printf(" HEDGE RNG <UNIFORM> UNIT TEST:\n");
    printf(" %22s: %ld\n", "samples", num_samples);
    printf(" %22s: %0.2lf\n", "min", min);
    printf(" %22s: %0.2lf\n", "max", max);
    printf("----------------------------------------------------------\n\n");

    rng_init_uniform(&rng, (unsigned)time(&t), min, max);
    for (int i = 0; i < num_samples; i++) {
        r = rng_get(&rng);
    }
    rng_print_statistics(&rng);
    printf("----------------------------------------------------------\n\n");
}

static int unittest(void)
{
    rng_test(0, 3.0, 10000000);
    
    rng_test(0, 100, 100000);

    rng_test(100, 1.4142, 100000);

    rng_test_uniform(-10, 10, 100000);
    
    return 0;
}

__attribute__((weak)) int main(int argc, char **argv)
{
    unittest();
    return 0;
}
