#ifndef DROP_H
#define DROP_H

#include "tpp.h"
#include "tpp_solution.h"
#include "logging.h"


/**
 * This is an impl. of a "drop" heuristic in which a market is dropped from the
 * tour as soon as the decrease in traveling costs is greater than the increase
 * in purchase costs.
 *
 * The 'solution' is modified in place if an improvement is found.
 *
 * Returns a change in total solution cost, i.e. improvement.
 */
int drop_heuristic(const TPP::Instance &instance, TPP::Solution &solution);


int drop_heuristic_randomized(const TPP::Instance &instance, TPP::Solution &solution);

/**
 * Insertion heuristic tries to insert a new market into the route if the
 * increase in travel cost is lower than the decrease in purchase costs.
 */
int insertion_heuristic(const TPP::Instance &instance, TPP::Solution &solution);


int exchange_heuristic(const TPP::Instance &instance, TPP::Solution &solution);

/**
 * This is similar to the exchange_heuristic but drops two consecutive markets
 * from the tour and tries to insert a single one from the unvisited.
 */
int double_exchange_heuristic(const TPP::Instance &instance, TPP::Solution &sol);

/**
 * This is similar to the exchange_heuristic but drops two consecutive markets
 * from the tour and tries to insert a single one from the unvisited.
 *
 * This looks for a pair of markets to remove from route in a RANDOM order.
 */
int double_exchange_heuristic_r(const TPP::Instance &instance, TPP::Solution &sol);

/**
 * This is similar to the exchange_heuristic but drops k consecutive markets
 * from the tour and tries to insert a single one from the unvisited.
 */
int k_exchange_heuristic(const TPP::Instance &instance, TPP::Solution &sol,
                         const uint32_t k);



/**
 * Insertion heuristic tries to insert a new market into the route if the
 * increase in travel cost is lower than the decrease in purchase costs.
 */
template<typename ChangeAcceptFn>
int insertion_heuristic(const TPP::Instance &instance, TPP::Solution &solution,
                        ChangeAcceptFn accept) {
    LOG_SCOPE_F(INFO, "insertion_heuristic");

    auto &route = solution.route_;
    auto total_cost_change = 0;
    const auto &candidates = solution.get_unselected_markets();

    for (auto cand : candidates) {
        const auto verdict = solution.calc_market_add_cost(cand);
        if (accept(verdict.cost_change_)) {
            const auto prev_cost = solution.cost_;
            total_cost_change += verdict.cost_change_;

            solution.insert_market_at_pos(cand, verdict.index_);
            CHECK_F(prev_cost + verdict.cost_change_ == solution.cost_,
                    "Updated cost should be OK, %d", solution.cost_);
            CHECK_F(is_solution_valid(instance, solution.route_),
                    "Sol should be valid");
        }
    }
    CHECK_F(is_solution_valid(instance, route), "Sol. should be valid");
    CHECK_F(solution.is_valid(), "Sol. should be valid");
    LOG_F(INFO, "Total cost change: %d", total_cost_change);
    LOG_F(INFO, "New sol. cost: %d", solution.cost_);
    return total_cost_change;
}

#endif
