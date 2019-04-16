#ifndef TPP_SOLUTION_H
#define TPP_SOLUTION_H


#include "tpp.h"

namespace TPP {

    struct Solution {
        struct MarketAddVerdict {
            int cost_change_{ 0 };
            uint32_t index_{ 0 };
            bool demand_satisfied_{ false };  // True if after market is added
                                              // all demands are satisfied
        };

        const Instance &instance_;
        vector<uint32_t> route_{ 0u };  // A depot
        int cost_{ 0 };
        int travel_cost_{ 0 };
        vector<uint8_t> market_selected_;  // [i] = true if market i is a part of solution
        vector<vector<ProductOffer>> product_offers_; // [i] - list of offers for product i
                                                      // in the solution
        vector<int> purchase_costs_; // [i] = total purchase cost for product i
        vector<int> demand_remaining_; // [i] = a total unsatisfied demand for the product i
        vector<uint32_t> remaining_products_; // a list of ids of still needed
                                            // products, i.e. with demand > 0
        vector<uint32_t> markets_per_product_;  // [i] = how many markets were
                                              // necessary to satisfy demand
        vector<uint32_t> unselected_markets_;
        int total_unsatisfied_demand_{ 0 };

        Solution(const Instance &instance) noexcept;

        void push_back_market(uint32_t market_id) noexcept;

        void insert_market_at_pos(uint32_t market_id, uint32_t index) noexcept;

        void remove_market_at_pos(uint32_t pos) noexcept;

        /**
        * Calculates how the solution cost changes if a product offer is added.
        * Returns change of a total product purchase cost and a boolean equal to true
        * if demand is satisfied after adding the offer.
        *
        * This has complexity of O(log(K)) for U-TPP
        * and O(M log(K)) for capacitated version
        */
        pair<int, int> calc_product_offer_add_cost(ProductOffer offer) const noexcept;

        int add_product_offer(const ProductOffer &offer) noexcept;

        int remove_product_offer(const ProductOffer &offer) noexcept;

        pair<int, bool> calc_product_offer_removal_cost(const ProductOffer &offer) const noexcept;

        MarketAddVerdict calc_market_removal_cost(uint32_t market_id,
                bool validity_required=false) const noexcept;

        MarketAddVerdict calc_market_add_cost(uint32_t market_id) const noexcept;

        /*
         * Returns true if adding market to a solution will result in a
         * feasible solution, i.e. with all demands satisfied.
         */
        bool check_market_satisfies_demand(uint32_t market_id) const noexcept;

        bool is_market_used(uint32_t market_id) const noexcept;

        bool is_valid() const noexcept;

        vector<uint32_t> get_unselected_markets() const noexcept;

        uint32_t get_market_pos_in_route(uint32_t market_id) const noexcept;

        /**
         * Returns an error of the solution relative to the best known result
         * or std::numeric_limits<double>::max() if best known is not
         * available.
         */
        double get_relative_error() const noexcept;
    };

}


#endif
