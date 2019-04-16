#ifndef VEC_H
#define VEC_H

#include <vector>


namespace Vec {
/*
 * Calculates a sum of max(0, a[i] - b[i]) for i = 0, 1, ... , a.size()-1
 */
int calc_diff_max_0_sum(const std::vector<int> &a,
                        const std::vector<int> &b);

void run_tests();

}

#endif
