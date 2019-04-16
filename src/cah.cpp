#include <numeric>
#include <cmath>

#include "cah.h"
#include "drop.h"
#include "logging.h"
#include "two_opt.h"
#include "three_opt.h"
#include "rand.h"


using namespace TPP;
using namespace std;


/**
 * This is an attempt to implement commodity adding heuristic, i.e. CAH
 * as described in
 * Boctor, Fayez F., Gilbert Laporte, and Jacques Renaud. "Heuristics for the
 * traveling purchaser problem." Computers & Operations Research 30.4 (2003):
 * 491-504.
 */
TPP::Solution commodity_adding_heuristic(const TPP::Instance &instance) {
    LOG_SCOPE_F(INFO, "CAH");

    Solution sol(instance);
    LOG_F(INFO, "Req. demand: %d, start sol: %d",
          instance.demands_.at(1), sol.demand_remaining_.at(1));

    vector<size_t> products(instance.product_count_);
    iota(begin(products), end(products), 0);

    shuffle_vector(products);

    auto h0 = products.front();

    // Determine the market for which the unit purchase of product h0 is
    // minimized
    size_t best_market = 0u;
    double best_offer = numeric_limits<double>::max();
    for (const auto &market_offers : instance.market_product_offers_) {
        const auto offer = market_offers.at(h0); // offer for product h0
        if (offer.quantity_ == 0) {
            continue ;
        }
        const auto market_id = offer.market_id_;
        const auto value = 2 * instance.get_travel_cost(0, market_id)
                         / static_cast<double>(offer.quantity_) + offer.price_;
        if (value < best_offer) {
            best_offer = value;
            best_market = market_id;
        }
    }
    CHECK_F(best_market != 0, "Best market should not be the depot");
    sol.push_back_market(best_market);

    for (auto h : products) {
        while (sol.demand_remaining_.at(h) > 0) {
            int min_cost = numeric_limits<int>::max();
            best_market = 0u;
            Solution::MarketAddVerdict verdict;

            for (auto m = 1u; m < instance.dimension_; ++m) {
                if (sol.market_selected_.at(m)
                        || instance.market_product_offers_.at(m).at(h).quantity_ == 0) {
                    continue ;
                }

                const auto res = sol.calc_market_add_cost(m);
                if (res.cost_change_ < min_cost) {
                    min_cost = res.cost_change_;
                    best_market = m;
                    verdict = res;
                }
            }
            CHECK_F(best_market != 0, "Best market cannot be depot");
            sol.insert_market_at_pos(best_market, verdict.index_);
        }
    }
    LOG_F(INFO, "Cost before LS: %d", sol.cost_);
    CHECK_F(is_solution_valid(instance, sol.route_), "Sol should be valid");
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    auto &rng = get_random_engine();
    auto improvement_found = false;
    do {
        improvement_found = false;
        const auto start_cost = sol.cost_;

        drop_heuristic(instance, sol);
        insertion_heuristic(instance, sol);
        // double_exchange_heuristic(instance, sol);
        exchange_heuristic(instance, sol);
        three_opt_nn(instance, sol, /*don't look bits=*/true, /*nn_count=*/25);

        if (sol.cost_ < start_cost) {
            improvement_found = true;
        }
    } while(improvement_found);

    CHECK_F(is_solution_valid(instance, sol.route_), "Sol should be valid");
    LOG_F(INFO, "Final cost: %d", sol.cost_);

    return sol;
}
