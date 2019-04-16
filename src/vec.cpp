#include "vec.h"
#include "logging.h"

//#include <x86intrin.h>
#include <cmath>

using namespace Vec;
using namespace std;


int calc_diff_max_0_sum_naive(const std::vector<int> &a,
                              const std::vector<int> &b) {
    int result = 0;
    for (size_t i = 0, len = a.size() ; i < len; ++i) {
        result += std::max(0, a[i] - b[i]);
    }
    return result;
}


/*
 * Calculates a sum of max(0, a[i] - b[i]) for i = 0, 1, ... , a.size()-1
 */
int Vec::calc_diff_max_0_sum(const std::vector<int> &a,
                             const std::vector<int> &b) {
    return calc_diff_max_0_sum_naive(a, b);
}


void test_calc_fast_diff_max_0_sum() {
    vector<int> a { 1, 2, 3, 4, 1, 2, 3, 4, 7, 8 };
    vector<int> b { 0, 1, 2, 3, 2, 3, 4, 5, 7, 8 };

    auto r = Vec::calc_diff_max_0_sum(a, b);
    CHECK_F(r == 4, "Sum should be 4 but is: %d", r);
}


void Vec::run_tests() {
    test_calc_fast_diff_max_0_sum();
}
