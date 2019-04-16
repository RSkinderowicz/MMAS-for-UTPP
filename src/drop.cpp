#include <limits>
#include <algorithm>
#include <random>

#include "drop.h"
#include "logging.h"
#include "rand.h"

using namespace std;
using namespace TPP;


/**
 * This is an impl. of a "drop" heuristic in which a market is dropped from the
 * tour as soon as the decrease in traveling costs is greater than the increase
 * in purchase costs.
 *
 * The 'solution' is modified in place if an improvement is found.
 *
 * Returns a change in total solution cost, i.e. improvement.
 *
 * The complexity is O(M^2 * K) for the U-TPP but if no market is
 * dropped it will perform only O(M * max(K, M)) operations.
 */
int drop_heuristic(const TPP::Instance &instance, TPP::Solution &solution) {
    //LOG_SCOPE_F(INFO, "drop_heuristic");
    const auto start_cost = solution.cost_;
    auto solution_changed = false;
    for (auto i = 1u; i < solution.route_.size(); ++i) {
        const auto market_id = solution.route_.at(i);
        const auto after = solution.calc_market_removal_cost(market_id,
                /*validity_required=*/true);

        if (after.demand_satisfied_ && after.cost_change_ < 0) {
            solution.remove_market_at_pos(i);
            solution_changed = true;
            --i;
        }
    }
    if (solution_changed) {
        CHECK_F(is_solution_valid(instance, solution.route_),
                "Sol. should be valid");
    }
    return solution.cost_ - start_cost;
}


int drop_heuristic_randomized(const TPP::Instance &instance, TPP::Solution &solution) {
    auto &route = solution.route_;
    vector<size_t> markets(begin(route) + 1, end(route));
    shuffle_vector(markets);

    const auto start_cost = solution.cost_;
    auto solution_changed = false;

    for (auto market_id : markets) {
        const auto after = solution.calc_market_removal_cost(market_id,
                /*validity_required=*/true);

        if (after.demand_satisfied_
                && after.cost_change_ < 0) {
            solution.remove_market_at_pos(solution.get_market_pos_in_route(market_id));
            solution_changed = true;
        }
    }
    if (solution_changed) {
        CHECK_F(is_solution_valid(instance, solution.route_),
                "Sol. should be valid");
    }
    return solution.cost_ - start_cost;
}


/**
 * Insertion heuristic tries to insert a new market into the route if the
 * increase in travel cost is lower than the decrease in purchase costs.
 */
int insertion_heuristic(const TPP::Instance &instance, TPP::Solution &solution) {
    LOG_SCOPE_F(INFO, "insertion_heuristic");

    auto &route = solution.route_;
    auto total_cost_change = 0;
    const auto &candidates = solution.get_unselected_markets();
    auto solution_changed = false;

    for (auto cand : candidates) {
        const auto verdict = solution.calc_market_add_cost(cand);
        //LOG_F(INFO, "Cost of adding %zu is %d at pos %zu",
                //cand, verdict.first,
                //verdict.second);
        if (verdict.cost_change_ <= 0) {
            const auto prev_cost = solution.cost_;
            total_cost_change += verdict.cost_change_;

            solution.insert_market_at_pos(cand, verdict.index_);
            CHECK_F(prev_cost + verdict.cost_change_ == solution.cost_,
                    "Updated cost should be OK, %d", solution.cost_);
            solution_changed = true;
        }
    }
    if (solution_changed) {
        CHECK_F(is_solution_valid(instance, route),
                "Sol. should be valid");
    }
    LOG_F(INFO, "Total cost change: %d", total_cost_change);
    LOG_F(INFO, "New sol. cost: %d", solution.cost_);
    return total_cost_change;
}


/**
 * This heuristic drops a market from the solution and tries to insert one of
 * the unvisited ones as long as it yields cost reduction while maintaining
 * feasibility.
 */
int exchange_heuristic(const TPP::Instance &instance, TPP::Solution &sol) {
    LOG_SCOPE_F(INFO, "exchange_heuristic");

    LOG_F(INFO, "Start cost: %d", sol.cost_);

    auto total_cost_change = 0;
    auto unselected = sol.get_unselected_markets();
    auto solution_changed = false;

    const vector<size_t> markets_to_check(sol.route_.begin() + 1, sol.route_.end());

    for (const auto market_id : markets_to_check) {
        const auto cost_before_removal = sol.cost_;
        const auto market_pos = sol.get_market_pos_in_route(market_id);
        sol.remove_market_at_pos(market_pos);

        bool found = false;
        // Look for another market to insert
        for (auto cand : unselected) {
            if ( !sol.check_market_satisfies_demand(cand) ) {
                continue ;
            }

            const auto verdict = sol.calc_market_add_cost(cand);

            if (sol.cost_ + verdict.cost_change_ <= cost_before_removal
                    && verdict.demand_satisfied_) {
                LOG_F(INFO, "Cost of adding %zu is %d at pos %zu",
                        cand, verdict.cost_change_, verdict.index_);
                const auto prev_cost = sol.cost_;
                total_cost_change += verdict.cost_change_;

                sol.insert_market_at_pos(cand, verdict.index_);

                CHECK_F(prev_cost + verdict.cost_change_ == sol.cost_,
                        "Updated cost should be OK, %d", sol.cost_);
                LOG_F(INFO, "Cost now: %d", sol.cost_);
                unselected.erase(find(begin(unselected), end(unselected), cand));
                found = true;
                break ;
            }
        }
        if (!found) { // Restore solution to the previous state
            sol.insert_market_at_pos(market_id, market_pos);
        } else {
            solution_changed = true;
        }
    }
    if (solution_changed) {
        CHECK_F(is_solution_valid(instance, sol.route_),
                "Sol. should be valid");
    }
    return total_cost_change;
}


/**
 * This is similar to the exchange_heuristic but drops two consecutive markets
 * from the tour and tries to insert a single one from the unvisited.
 */
int double_exchange_heuristic(const TPP::Instance &instance, TPP::Solution &sol) {
    LOG_SCOPE_F(INFO, "double_exchange_heuristic");

    LOG_F(INFO, "Start cost: %d", sol.cost_);

    auto total_cost_change = 0;
    auto solution_changed = false;
    auto unselected = sol.get_unselected_markets();

    const auto route_copy = sol.route_;

    for (auto i = 1u; i + 1 < route_copy.size(); ++i) {
        const auto cost_before_removal = sol.cost_;
        const auto market_1 = route_copy.at(i);
        const auto market_2 = route_copy.at(i + 1);

        const auto pos_1 = sol.get_market_pos_in_route(market_1);
        const auto pos_2 = sol.get_market_pos_in_route(market_2);

        CHECK_F(pos_1 != sol.route_.size(), "pos_1 out of bounds");
        CHECK_F(pos_2 != sol.route_.size(), "pos_1 out of bounds");

        if (pos_1 < pos_2) {
            sol.remove_market_at_pos(pos_2);
            sol.remove_market_at_pos(pos_1);
        } else {
            sol.remove_market_at_pos(pos_1);
            sol.remove_market_at_pos(pos_2);
        }
        bool found = false;
        // Look for another market to insert
        for (auto cand : unselected) {
            if ( !sol.check_market_satisfies_demand(cand) ) {
                continue ;
            }
            const auto verdict = sol.calc_market_add_cost(cand);

            if (sol.cost_ + verdict.cost_change_ < cost_before_removal
                    && verdict.demand_satisfied_) {
                LOG_F(INFO, "Cost of adding %zu is %d at pos %zu",
                        cand, verdict.cost_change_, verdict.index_);
                const auto prev_cost = sol.cost_;
                total_cost_change += verdict.cost_change_;

                sol.insert_market_at_pos(cand, verdict.index_);

                CHECK_F(prev_cost + verdict.cost_change_ == sol.cost_,
                        "Updated cost should be OK, %d", sol.cost_);
                LOG_F(INFO, "Cost now: %d", sol.cost_);
                unselected.erase(find(begin(unselected), end(unselected), cand));
                found = true;
                break ;
            }
        }
        if (found) {
            solution_changed = true;
            ++i; // Skip over market_2
        } else { // Restore solution to the previous state
            if (pos_1 < pos_2) {
                sol.insert_market_at_pos(market_1, pos_1);
                sol.insert_market_at_pos(market_2, pos_2);
            } else {
                sol.insert_market_at_pos(market_2, pos_2);
                sol.insert_market_at_pos(market_1, pos_1);
            }
        }
    }
    LOG_F(INFO, "Final cost: %d", sol.cost_);
    if (solution_changed) {
        CHECK_F(is_solution_valid(instance, sol.route_),
                "Sol. should be valid");
    }
    return total_cost_change;
}


/**
 * This is similar to the exchange_heuristic but drops two consecutive markets
 * from the tour and tries to insert a single one from the unvisited.
 *
 * This looks for a pair of markets to remove from route in a RANDOM order.
 */
int double_exchange_heuristic_r(const TPP::Instance &instance, TPP::Solution &sol) {
    LOG_SCOPE_F(INFO, "double_exchange_heuristic_r");

    LOG_F(INFO, "Start cost: %d", sol.cost_);

    auto total_cost_change = 0;
    auto solution_changed = false;
    auto unselected = sol.get_unselected_markets();

    //const auto route_copy = sol.route_;
    vector<size_t> route_copy(sol.route_.begin() + 1, sol.route_.end());
    shuffle_vector(route_copy);

    for (auto i = 0u; i + 1 < route_copy.size(); ++i) {
        const auto cost_before_removal = sol.cost_;
        const auto market_1 = route_copy.at(i);
        const auto market_2 = route_copy.at(i + 1);

        const auto pos_1 = sol.get_market_pos_in_route(market_1);
        const auto pos_2 = sol.get_market_pos_in_route(market_2);

        CHECK_F(pos_1 != sol.route_.size(), "pos_1 out of bounds");
        CHECK_F(pos_2 != sol.route_.size(), "pos_1 out of bounds");

        if (pos_1 < pos_2) {
            sol.remove_market_at_pos(pos_2);
            sol.remove_market_at_pos(pos_1);
        } else {
            sol.remove_market_at_pos(pos_1);
            sol.remove_market_at_pos(pos_2);
        }
        bool found = false;
        // Look for another market to insert
        for (auto cand : unselected) {
            const auto verdict = sol.calc_market_add_cost(cand);

            if (sol.cost_ + verdict.cost_change_ < cost_before_removal
                    && verdict.demand_satisfied_) {
                const auto prev_cost = sol.cost_;
                total_cost_change += verdict.cost_change_;

                sol.insert_market_at_pos(cand, verdict.index_);

                CHECK_F(prev_cost + verdict.cost_change_ == sol.cost_,
                        "Updated cost should be OK, %d", sol.cost_);
                unselected.erase(find(begin(unselected), end(unselected), cand));
                found = true;
                break ;
            }
        }
        if (found) {
            solution_changed = true;
            ++i; // Skip over market_2
        } else { // Restore solution to the previous state
            if (pos_1 < pos_2) {
                sol.insert_market_at_pos(market_1, pos_1);
                sol.insert_market_at_pos(market_2, pos_2);
            } else {
                sol.insert_market_at_pos(market_2, pos_2);
                sol.insert_market_at_pos(market_1, pos_1);
            }
        }
    }
    if (solution_changed) {
        CHECK_F(is_solution_valid(instance, sol.route_),
                "Sol. should be valid");
    }
    return total_cost_change;
}


/**
 * This is similar to the exchange_heuristic but drops k consecutive markets
 * from the tour and tries to insert a single one from the unvisited.
 */
int k_exchange_heuristic(const TPP::Instance &instance, TPP::Solution &sol,
                         const uint32_t k) {
    LOG_SCOPE_F(INFO, "k_exchange_heuristic");

    LOG_F(INFO, "Start cost: %d", sol.cost_);

    auto total_cost_change = 0;
    auto unselected = sol.get_unselected_markets();
    auto solution_changed = false;

    const auto route_copy = sol.route_;

    struct MarketPosition {
        size_t market_{ 0 };
        size_t position_{ 0 };

        bool operator<(const MarketPosition &rhs) {
            return this->position_ < rhs.position_;;
        }
    };

    vector<MarketPosition> rem_markets;
    rem_markets.reserve(k);

    for (auto i = 1u; i + k - 1 < route_copy.size(); ++i) {
        const auto cost_before_removal = sol.cost_;

        rem_markets.clear();
        for (auto j = 0u; j < k; ++j) {
            const auto market = route_copy.at(i + j);
            const auto pos = sol.get_market_pos_in_route(market);
            rem_markets.push_back(MarketPosition{ market, pos });
        }
        sort(begin(rem_markets), end(rem_markets));

        for (auto it = rem_markets.rbegin(); it != rem_markets.rend(); ++it) {
            const auto &el = *it;
            sol.remove_market_at_pos(el.position_);
        }
        bool found = false;
        // Look for another market to insert
        for (auto cand : unselected) {
            if ( !sol.check_market_satisfies_demand(cand) ) {
                continue ;
            }
            const auto verdict = sol.calc_market_add_cost(cand);

            if (sol.cost_ + verdict.cost_change_ < cost_before_removal
                    && verdict.demand_satisfied_) {
                LOG_F(INFO, "Cost of adding %zu is %d at pos %zu",
                        cand, verdict.cost_change_, verdict.index_);
                const auto prev_cost = sol.cost_;
                total_cost_change += verdict.cost_change_;

                sol.insert_market_at_pos(cand, verdict.index_);

                CHECK_F(prev_cost + verdict.cost_change_ == sol.cost_,
                        "Updated cost should be OK, %d", sol.cost_);
                LOG_F(INFO, "Cost now: %d", sol.cost_);
                unselected.erase(find(begin(unselected), end(unselected), cand));
                found = true;
                break ;
            }
        }
        if (found) {
            solution_changed = true;
            i += k - 1; // Skip over removed markets
        } else { // Restore solution to the previous state
            for (auto it = rem_markets.begin(); it != rem_markets.end(); ++it) {
                const auto &el = *it;
                sol.insert_market_at_pos(el.market_, el.position_);
            }
        }
    }
    LOG_F(INFO, "Final cost: %d", sol.cost_);
    if (solution_changed) {
        CHECK_F(is_solution_valid(instance, sol.route_),
                "Sol. should be valid");
    }
    return total_cost_change;
}
