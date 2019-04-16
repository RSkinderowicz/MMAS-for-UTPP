#include <vector>
#include <limits>
#include <algorithm>
#include <numeric>

#include "logging.h"
#include "gsh.h"
#include "vec.h"


using namespace TPP;
using namespace std;



/**
 * This creates a matrix M of market and product prices, i.e. M[i][j] denotes a
 * price of product j in market i. If market does not provide a product than
 * the corresponding entry has value equal to "super price", i.e. a special
 * high value returned through out_super_price param.
 */
vector<vector<int>> calc_market_product_prices(const TPP::Instance& instance,
        int *out_super_price = nullptr,
        bool filter_not_required_products = true) {
    vector<vector<int>> market_product_prices;

    market_product_prices.resize(instance.dimension_);
    auto market_id = 0u;
    auto max_price = 0;
    for (const auto &offers: instance.market_offers_) {
        auto &prices = market_product_prices.at(market_id);

        // max val. will be replaced with a smaller value later
        prices.resize(instance.product_count_, numeric_limits<int>::max());

        for (auto &offer: offers) {
            prices.at(static_cast<size_t>(offer.product_id_)) = offer.price_;
            max_price = max(max_price, offer.price_);
        }
        ++market_id;
    }
    int max_weight = 0;
    for (const auto &weights: instance.edge_weights_) {
        max_weight = max(max_weight,
                         *max_element(begin(weights), end(weights)));
    }
    // A price value that is >> max(price, edge weight)
    const auto max_demand = *max_element(begin(instance.demands_),
                                         end(instance.demands_));
    // super_price is used to denote that a market does not provide a product
    // It can be used in calculations more conveniently than
    // numeric_limits<int>::max() which would cause an overflow if added to a
    // non-zero value
    const auto super_price = static_cast<int>(instance.dimension_ + instance.product_count_)
                           * max(max_weight, max_price)
                           * max_demand;
    // Make sure that super_price will not cause overflow in later calculations
    assert(static_cast<int64_t>(numeric_limits<int32_t>::max()) > super_price
                         * static_cast<int64_t>(max_demand)
                         * static_cast<int>(instance.product_count_)
                         + static_cast<int>(instance.dimension_)
                         * max_weight );
    if (out_super_price) {
        *out_super_price = super_price;
    }
    // Remove columns for the products which are not needed, i.e. demand is 0
    if (filter_not_required_products) {
        for (auto &prices : market_product_prices) {
            vector<int> just_required;
            just_required.reserve(instance.needed_products_.size());
            for (auto p : instance.needed_products_) {
                just_required.push_back(prices.at(p));
            }
            prices = just_required;  // replace only with required product prices
        }
    }

    LOG_F(INFO, "Max price: %d, max weight: %d, super price: %d",
            max_price, max_weight, super_price);

    for (auto &prices: market_product_prices) {
        replace(begin(prices), end(prices),
                numeric_limits<int>::max(), super_price); // old -> super_price
    }
    return market_product_prices;
}


/*
 * This assumes that TPP instance is uncapacitated, i.e. each supplier (market)
 * has unlimited amount of a product or does not offer it at all.
 */
TPP::Solution calc_gsh_solution(const TPP::Instance& instance) {
    CHECK_F(instance.is_capacitated_ == false,
            "Uncapacitated TPP instance required");

    LOG_SCOPE_F(INFO, "calc_gsh_solution");

    int super_price = -1;
    const auto market_product_prices = calc_market_product_prices(instance, &super_price);

    TPP::Solution sol(instance);

    // Find a market with most products at the lowest total price
    int max_products = 0;
    int min_total_cost = numeric_limits<int>::max();
    size_t chosen_market_id = 0;
    size_t market_id = 0;
    for (const auto &prices: market_product_prices) {
        int products_available = 0;
        int total_cost = 0;
        for (auto price : prices) {
            if (price != super_price) {
                ++products_available;
                total_cost += price;
            }
        }
        if (products_available > max_products
            || (products_available == max_products
                && min_total_cost > total_cost)) {
            max_products = products_available;
            min_total_cost = total_cost;
            chosen_market_id = market_id;
        }
        ++market_id;
    }
    LOG_F(INFO, "market: %zu, max_products: %d, min_total_cost: %d",
          chosen_market_id, max_products, min_total_cost);

    vector<size_t> unselected(instance.dimension_ - 1);
    iota(begin(unselected), end(unselected), 1u);  // 1, 2, ..., dimension - 1
    // Remove chosen_market_id
    unselected.erase(find(begin(unselected), end(unselected), chosen_market_id));

    sol.push_back_market(chosen_market_id);

    // [i] = price for product i
    vector<int> prices_in_sol{ market_product_prices.at(chosen_market_id) };
    auto improvement_found = false;
    do {
        improvement_found = false;

        auto best_i = 0u;
        auto best_h = 0u;
        auto best_savings = 0;

        auto i = sol.route_.back();
        for (auto j : sol.route_) {
            const auto c_ij = instance.edge_weights_.at(i).at(j);

            // Look for customer h that can be inserted between (i, j)
            for (auto h : unselected) {
                auto c_ih = instance.edge_weights_.at(i).at(h);
                auto c_hj = instance.edge_weights_.at(h).at(j);
                const auto &h_prices = market_product_prices.at(h);

                //auto diff_sum = 0;
                //for (auto k = 0ul, end = prices_in_sol.size(); k < end; ++k) {
                    //diff_sum += max(0, prices_in_sol.at(k) - h_prices.at(k));
                //}
                auto diff_sum = Vec::calc_diff_max_0_sum(prices_in_sol, h_prices);
                auto savings = c_ij - c_ih - c_hj + diff_sum;

                if (savings > best_savings) {
                    best_i = i;
                    best_h = h;
                    best_savings = savings;
                }
            }
            i = j;
        }
        if (best_savings > 0) {
            LOG_F(INFO, "Savings found: %d for h: %u after i: %d", best_savings, best_h, best_i);
            improvement_found = true;

            auto it = find(begin(sol.route_), end(sol.route_), best_i);
            ++it;
            int index = distance(begin(sol.route_), it);
            sol.insert_market_at_pos(best_h, static_cast<size_t>(index));
            // route.insert(pos, best_h);
            unselected.erase(find(begin(unselected), end(unselected), best_h));

            const auto &h_prices = market_product_prices.at(best_h);
            for (auto k =0u; k < instance.product_count_; ++k) {
                prices_in_sol.at(k) = min(prices_in_sol.at(k), h_prices.at(k));
            }
        }
    } while(improvement_found);

    assert(is_solution_valid(instance, sol.route_));
    LOG_F(INFO, "Autocalculated sol cost: %d", sol.cost_);
    auto cost = calc_solution_cost(instance, sol.route_);
    LOG_F(INFO, "Final GSH sol cost: %d", cost);
    LOG_F(INFO, "Final GSH sol: %s", container_to_string(sol.route_).c_str());
    sol.cost_ = cost;
    return sol;
}
