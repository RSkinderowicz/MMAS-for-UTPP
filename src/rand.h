#ifndef RAND_H
#define RAND_H

#include <vector>
#include <random>
#include <algorithm>


uint32_t get_initial_seed();


void set_initial_seed(uint32_t value);


/**
 * This is based on xoroshiro+ algorithm by David Blackman and Sebastiano Vigna
 * as described at: http://xoroshiro.di.unimi.it/
 */
struct xoroshiro128plus {
    typedef uint64_t result_type;

    uint64_t state_[2];

    xoroshiro128plus();

    uint64_t next(void) noexcept;

    uint64_t operator()() noexcept { return next(); }

    uint64_t min() const noexcept { return 0u; }

    uint64_t max() const noexcept { return std::numeric_limits<uint64_t>::max(); }
};


xoroshiro128plus& get_random_engine();


template<typename T>
void shuffle_vector(std::vector<T> &vec) {
    std::shuffle(vec.begin(), vec.end(), get_random_engine());
}


/*
 * Returns uniform random value in range [0, 1)
 */
inline double get_random_value() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(get_random_engine());
}


/**
 * Returns a number in range [min, max] drawn randomly with uniform
 * probability.
 */
inline double get_random_uint(uint32_t min, uint32_t max) {
    static std::uniform_int_distribution<uint32_t> distribution(min, max);
    return distribution(get_random_engine());
}


/**
 * Returns a random sample of sample_size numbers from 0 to n-1.
 */
std::vector<uint32_t> random_sample(uint32_t n, uint32_t sample_size);


#endif
