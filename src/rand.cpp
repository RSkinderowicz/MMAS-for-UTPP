#include <chrono>
#include <random>

#include "rand.h"
#include "logging.h"


using namespace std;


uint32_t initial_seed = 0;


uint32_t get_initial_seed() {
    if (::initial_seed == 0) {
        auto t = chrono::system_clock::now().time_since_epoch().count();
        ::initial_seed = t;
    }
    return initial_seed;;
}


void set_initial_seed(uint32_t value) {
    CHECK_F(value != 0, "Initial seed value needs to be > 0");
    ::initial_seed = value;
}


/**
 * Initializes generator using std::default_random_engine seeded with current
 * time.
 */
xoroshiro128plus::xoroshiro128plus() {
    const auto seed = get_initial_seed();
    auto rnd_engine = default_random_engine(seed);
    state_[0] = rnd_engine();
    state_[1] = rnd_engine();
}


static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}


uint64_t xoroshiro128plus::next() noexcept {
    const uint64_t s0 = state_[0];
    uint64_t s1 = state_[1];
    const uint64_t result = s0 + s1;

    s1 ^= s0;
    state_[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14); // a, b
    state_[1] = rotl(s1, 36); // c

    return result;
}



xoroshiro128plus& get_random_engine() {
    static xoroshiro128plus engine;
    return engine;
}


/**
 * Returns a random sample of sample_size numbers from 0 to n-1.
 * TODO this is inefficient - complexity is O(n) instead of O(sample_size)
 */
std::vector<uint32_t> random_sample(uint32_t n, uint32_t sample_size) {
    if (sample_size > n) {
        sample_size = n;
    }
    vector<uint32_t> sample(sample_size);
    for (auto i = 0u; i < sample_size; ++i) {
        sample[i] = i;
    }
    for (auto i = sample_size; i < n; ++i) {
        std::uniform_int_distribution<uint32_t> dist{0, i};
        auto r = dist(get_random_engine());
        if (r < sample_size) {
            sample[r] = i;
        }
    }
    return sample;
}
