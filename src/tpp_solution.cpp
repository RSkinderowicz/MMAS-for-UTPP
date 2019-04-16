
#include <algorithm>

#include "tpp_solution.h"
#include "logging.h"
#include "utils.h"


using namespace std;
using namespace TPP;


TPP::Solution::Solution(const Instance &instance) noexcept
    : instance_(instance),
      market_selected_(instance.dimension_, false),
      product_offers_(instance.product_count_),
      purchase_costs_(instance.product_count_, 0),
      demand_remaining_(instance.demands_),
      markets_per_product_(instance.product_count_, 0) {

    route_.reserve(instance.dimension_);
    market_selected_[0] = true;  // A depot

    remaining_products_.reserve(instance_.product_count_);
    for (auto p = 0u; p < instance_.product_count_; ++p) {
        if (demand_remaining_.at(p) > 0) {
            remaining_products_.push_back(p);
        }
        total_unsatisfied_demand_ += instance.demands_.at(p);
    }
    unselected_markets_.resize(instance_.dimension_ - 1);
    for (auto i = 1u; i < instance_.dimension_; ++i) {
        unselected_markets_[i-1] = i;
    }
}


void TPP::Solution::push_back_market(uint32_t market_id) noexcept {
    insert_market_at_pos(market_id, route_.size());
}


/**
 * Inserts market at the given index into the route.
 *
 * This has complexity of O(K*M) for the uncapacitated TPP.
 */
void TPP::Solution::insert_market_at_pos(uint32_t market_id, uint32_t index) noexcept {
    CHECK_F(market_selected_.at(market_id) == false,
            "Multiple market (%u) visits are not allowed", market_id);
    CHECK_F(index > 0, "No insertion at pos 0 is allowed");

    const auto prev = route_[index - 1];
    const auto next = route_[index % route_.size()];

    if (index == route_.size()) {
        route_.push_back(market_id);
    } else {
        route_.insert(route_.begin() + static_cast<int>(index), market_id);
    }

    market_selected_.at(market_id) = true;

    const auto travel_cost_change = instance_.get_travel_cost(prev, market_id)
                                  + instance_.get_travel_cost(market_id, next)
                                  - instance_.get_travel_cost(prev, next);
    travel_cost_ += travel_cost_change;
    cost_ += travel_cost_change;

    for (const auto &offer : instance_.market_offers_.at(market_id)) {
        cost_ += add_product_offer(offer);
    }

    auto it = find(begin(unselected_markets_), end(unselected_markets_),
                   market_id);
    CHECK_F(it != end(unselected_markets_),
            "market_id should be in unselected_markets_");
    unselected_markets_.erase(it);
}


/**
 * Removes the market at 'pos' and updates the purchase part of the solution.
 *
 * This has O(M*K) complexity for the U-TPP
 */
void TPP::Solution::remove_market_at_pos(uint32_t pos) noexcept {
    CHECK_F(pos < route_.size(), "Invalid position in erase_market_at_pos");
    CHECK_F(pos > 0, "Cannot remove depot");

    const auto prev = route_[pos - 1];
    const auto removed = route_[pos];
    const auto next = route_[(pos + 1) % route_.size()];

    route_.erase(route_.begin() + static_cast<int>(pos));

    market_selected_.at(removed) = false;

    const auto travel_cost_change = instance_.get_travel_cost(prev, next)
                                  - instance_.get_travel_cost(prev, removed)
                                  - instance_.get_travel_cost(removed, next);
    travel_cost_ += travel_cost_change;
    cost_ += travel_cost_change;

    for (const auto &offer : instance_.market_offers_.at(removed)) {
        cost_ += remove_product_offer(offer);
    }
    unselected_markets_.push_back(removed);
}


/**
 * Calculates how the solution cost changes if a product offer is added.
 * Returns change of a total product purchase cost and a change in product
 * demand caused by using new_offer
 *
 * This has complexity of O(1) for the uncapacitated TPP.
 */
pair<int, int> TPP::Solution::calc_product_offer_add_cost(ProductOffer new_offer) const noexcept {
    CHECK_F(instance_.is_capacitated_ == false,
            "Uncapacitated TPP instance required");

    const auto product_id = new_offer.product_id_;
    const auto &offers = product_offers_.at(new_offer.product_id_);
    const auto prev_cost = purchase_costs_[product_id];
    int cost = prev_cost;
    int demand_satisfied = demand_remaining_[product_id];

    if (offers.empty() || offers.front().price_ > new_offer.price_) {
        cost = new_offer.price_;  // Accept offer
    }
    return make_pair(cost - prev_cost, demand_satisfied);
}


/**
 * Adds product offer to a solution and updates the solution cost if necessary
 * Returns change of a total product purchase cost.
 *
 * This has complexity of O(M) for the uncapacitated TPP.
 */
int TPP::Solution::add_product_offer(const ProductOffer &new_offer) noexcept {
    CHECK_F(instance_.is_capacitated_ == false,
            "Uncapacitated TPP instance required");

    const auto product_id = new_offer.product_id_;
    auto &offers = product_offers_.at(product_id);

    // We keep the list of offers sorted according to (price, quantity)
    insert_sorted(offers, new_offer, is_better_offer);   // O(M) complexity

    auto &cost = purchase_costs_[product_id];
    const auto prev_cost = cost;
    const auto demand_before = demand_remaining_[product_id];
    const bool demand_satisfied_before = (demand_before == 0);
    bool demand_satisfied_after = false;

    const auto &cheapest = offers.front();
    cost = cheapest.price_;
    demand_remaining_.at(product_id) = 0;
    demand_satisfied_after = true;
    markets_per_product_[product_id] = 1;

    total_unsatisfied_demand_ -= demand_before;
    CHECK_F(total_unsatisfied_demand_ >= 0, "Demand has to be >= 0");

    // Is the demand finally satisfied by the market's offer?
    if (demand_satisfied_after != demand_satisfied_before) {
        auto it = lower_bound(begin(remaining_products_),
                              end(remaining_products_),
                              product_id);
        if (it != end(remaining_products_) && *it == product_id) {
            remaining_products_.erase(it);
        }
    }
    return cost - prev_cost;
}


/**
 * It has O(M) complexity for the uncapacitated TPP.
 */
int TPP::Solution::remove_product_offer(const ProductOffer &offer) noexcept {
    CHECK_F(instance_.is_capacitated_ == false,
            "Uncapacitated TPP instance required");

    auto &offers = product_offers_.at(offer.product_id_);

    auto it = lower_bound(begin(offers), end(offers), offer, is_better_offer);
    while (*it != offer) {
        ++it;
    }
    CHECK_F(it != offers.end(), "Offer should exist in solution");

    offers.erase(it);

    auto &cost = purchase_costs_.at(offer.product_id_);
    const auto prev_cost = cost;
    bool demand_unsatisfied = false;

    if (!offers.empty()) {
        // Now we are using the next cheapest market
        const auto &cheapest = offers.front();
        cost = cheapest.price_;

        demand_remaining_[offer.product_id_] = 0;
        markets_per_product_[offer.product_id_] = 1;
    } else {  // No offers for the product
        cost = 0;
        demand_remaining_[offer.product_id_] = 1;
        demand_unsatisfied = true;
        markets_per_product_[offer.product_id_] = 0;
        total_unsatisfied_demand_ += 1;
        CHECK_F(total_unsatisfied_demand_ >= 0, "Demand has to be >= 0");
    }

    if (demand_unsatisfied) {
        auto it = lower_bound(begin(remaining_products_),
                              end(remaining_products_),
                              offer.product_id_);
        if (it == end(remaining_products_)) {
            remaining_products_.push_back(offer.product_id_);
        } else if (*it != offer.product_id_) {
            remaining_products_.insert(it, offer.product_id_);
        }
    }
    return cost - prev_cost;
}


/**
 * Calc. change in solution cost if product offer is removed from the solution.
 * Returns the change of sol. cost and a boolean denoting wether the demand for
 * the product is satisfied after the removal.
 *
 * Complexity is O(M) for the capacitated TPP and O(1) for the
 * uncapacitated TPP
 */
pair<int, bool>
TPP::Solution::calc_product_offer_removal_cost(const ProductOffer &rem_offer) const noexcept {
    CHECK_F(instance_.is_capacitated_ == false,
            "Uncapacitated TPP instance required");

    const auto product_id = rem_offer.product_id_;
    const auto &offers = product_offers_.at(product_id);
    int cost = 0;
    bool demand_satisfied = false;

    if (offers.size() >= 2) { // U-TPP - just use the next cheapest offer
        cost = offers[1].price_;  // use the next cheapest offer
        demand_satisfied = true;
    }
    const auto prev_cost = purchase_costs_[product_id];
    return make_pair(cost - prev_cost, demand_satisfied);
}

/**
 * Returns the cost of removing a market from a solution
 * and info about whether the solution will become invalid.
 *
 * If validity_required = true then we require that the solution is valid after
 * removal of the market thus we can stop the function as soon as we find that
 * removing one of the market's offers will render the solution invalid.
 *
 * This has O(max(K, M)) complexity for U-TPP and O(K*M) for the C-TPP.
 */
TPP::Solution::MarketAddVerdict
TPP::Solution::calc_market_removal_cost(uint32_t market_id,
        bool validity_required) const noexcept {
    const auto it = find(begin(route_), end(route_), market_id);
    CHECK_F(it != end(route_), "Market should be in the sol.");
    CHECK_F(it != begin(route_), "We cannot remove depot");

    bool all_demands_satisfied = (total_unsatisfied_demand_ == 0);
    int cost = 0;
    for (const auto &offer : instance_.market_offers_.at(market_id)) {
        const auto verdict = calc_product_offer_removal_cost(offer);
        if (validity_required && !verdict.second) {
            return MarketAddVerdict{ 0, 0, /*demand_satisfied=*/false };
        }
        cost += verdict.first;
        all_demands_satisfied &= verdict.second;
    }
    const auto index = distance(begin(route_), it);
    const auto prev = route_.at(static_cast<uint32_t>(index - 1));
    const auto curr = market_id;
    const auto next = route_.at(static_cast<uint32_t>(index + 1) % route_.size());

    // Distance (travel costs) change if we remove 'curr'
    const auto dist_decrease = instance_.get_travel_cost(prev, curr)
                             + instance_.get_travel_cost(curr, next)
                             - instance_.get_travel_cost(prev, next);
    return MarketAddVerdict{ cost - dist_decrease,
                             0,  /* index, not used here */
                             all_demands_satisfied };
}


/**
 * Returns a pair of cost change and an index at which the new market should be
 * inserted.
 *
 * If the returned cost is < 0 then it is profitable to insert the new market.
 *
 * This has O(max(K, M)) complexity for U-TPP and O(K*M) for the C-TPP.
 */
TPP::Solution::MarketAddVerdict
TPP::Solution::calc_market_add_cost(uint32_t market_id) const noexcept {
    CHECK_F(!is_market_used(market_id),
            "Market should not be in the sol.");

    auto unsatisfied_count = total_unsatisfied_demand_;
    int cost = 0;
    for (const auto &offer : instance_.market_offers_.at(market_id)) {
        const auto res = calc_product_offer_add_cost(offer);
        cost += res.first;
        unsatisfied_count -= res.second;
    }
    const bool all_demands_satisfied = (unsatisfied_count == 0);
    // Look for the cheapest place to insert the new market
    const auto len = route_.size();
    int min_dist_increase = numeric_limits<int>::max();
    uint32_t min_dist_index = len + 1;  // Best position for proposed market
    for (auto i = 0u; i < len; ++i) {
        const auto curr = route_[i];
        const auto next = route_[(i + 1) % len];
        const auto dist_increase = instance_.get_travel_cost(curr, market_id)
                                 + instance_.get_travel_cost(market_id, next)
                                 - instance_.get_travel_cost(curr, next);
        if (dist_increase < min_dist_increase) {
            min_dist_increase = dist_increase;
            min_dist_index = i + 1;
        }
    }
    return MarketAddVerdict{ cost + min_dist_increase,
                             min_dist_index,
                             all_demands_satisfied };
}


/*
 * Returns true if adding market to a solution will result in a
 * feasible solution, i.e. with all demands satisfied.
 *
 * The complexity is O(M)
 */
bool TPP::Solution::check_market_satisfies_demand(uint32_t market_id) const noexcept {
    if (is_market_used(market_id)) {
        return false;
    }
    const auto &offers = instance_.market_product_offers_.at(market_id);
    for (auto prod_id : remaining_products_) {
        if (offers[prod_id].quantity_ < demand_remaining_[prod_id]) {
            return false;  // market cannot satisfy demand for this product
        }
    }
    return true;
}


/**
 * Returns true if market is a part of solution.
 *
 * This has O(1) complexity.
 */
bool TPP::Solution::is_market_used(uint32_t market) const noexcept {
    return market_selected_.at(market);
}


/**
 * This has O(K) complexity.
 */
bool TPP::Solution::is_valid() const noexcept {
    return remaining_products_.empty();
}


/**
 * Return a vector with unselected markets, i.e. markets that are not part of
 * a solution.
 *
 * Complexity is O(M) as it requires making a copy of unselected_markets_ vec.
 */
vector<uint32_t>
TPP::Solution::get_unselected_markets() const noexcept {
    return unselected_markets_;
}


/**
 * Returns an index of a market in the solution or route_.size() if the
 * market is not present.
 *
 * Comlexity is O(M)
 */
uint32_t TPP::Solution::get_market_pos_in_route(uint32_t market_id) const noexcept {
    for (auto i = 0u; i < route_.size(); ++i) {
        if (route_[i] == market_id) {
            return i;
        }
    }
    return route_.size();
}


/**
 * Returns an error of the solution relative to the best known result
 * or std::numeric_limits<double>::max() if best known is not
 * available.
 */
double TPP::Solution::get_relative_error() const noexcept {
    double error = numeric_limits<double>::max();
    if (instance_.best_known_cost_ > 0) {
        error = (cost_ - instance_.best_known_cost_)
              / static_cast<double>(instance_.best_known_cost_);
    }
    return error;
}
