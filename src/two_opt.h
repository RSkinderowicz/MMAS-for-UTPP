#ifndef TWO_OPT_H
#define TWO_OPT_H

#include "tpp.h"
#include "tpp_solution.h"

#include <algorithm>


/*
 * A basic version of the 2-opt heuristic.
 *
 * Returns improvement over the previous route length
 */
int two_opt(const TPP::Instance &instance,
            std::vector<uint32_t> &route);

/*
 * A basic version of the 2-opt heuristic.
 *
 * Returns improvement over the previous route length
 */
int two_opt(const TPP::Instance &instance, TPP::Solution &sol);


/*
 * This is a randomized version of 2-opt in which the nodes are shuffled and
 * the 2-opt algorithm is restarted 'attempts' times.
 */
template<typename RandomEngine>
int two_opt_with_shuffle(const TPP::Instance &instance,
                         std::vector<uint32_t> &route,
                         RandomEngine &rnd,
                         uint32_t attempts=8) {

    const auto start_cost = calc_solution_cost(instance, route);
    auto best_cost = start_cost;
    std::vector<uint32_t> curr_route{ route };

    for (auto i = 0u; i < attempts; ++i) {
        auto improvement = two_opt(instance, curr_route);

        if (improvement > 0) {
            auto cost = calc_solution_cost(instance, curr_route);
            if (cost < best_cost) {
                best_cost = cost;
                route = curr_route;
            }
        }
        if (i + 1 < attempts) {
            // We do not touch the first node which is a depot
            std::shuffle(curr_route.begin() + 1, curr_route.end(), rnd);
        }
    }
    return start_cost - best_cost;
}


void test_two_opt();

#endif
