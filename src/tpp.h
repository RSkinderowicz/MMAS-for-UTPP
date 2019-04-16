#ifndef TPP_H
#define TPP_H


#include <string>
#include <vector>
#include <ostream>


namespace TPP {

    using std::string;
    using std::vector;
    using std::pair;


    struct ProductOffer {
        int price_{ 0 };
        int quantity_{ 0 };
        uint16_t product_id_{ 0 };
        uint16_t market_id_{ 0 };
    };


    inline bool has_lower_price(const ProductOffer& a, const ProductOffer &b) {
        return a.price_ < b.price_;
    }


    inline bool is_worse_offer(const ProductOffer& a, const ProductOffer &b) {
        return a.price_ > b.price_
            || (a.price_ == b.price_ && a.quantity_ < b.quantity_);
    }


    inline bool is_better_offer(const ProductOffer& a, const ProductOffer &b) {
        return a.price_ < b.price_
            || (a.price_ == b.price_ && a.quantity_ > b.quantity_);
    }


    inline bool operator==(const ProductOffer& a, const ProductOffer &b) {
        return a.market_id_ == b.market_id_ && a.product_id_ == b.product_id_;
    }


    inline bool operator!=(const ProductOffer& a, const ProductOffer &b) {
        return a.market_id_ != b.market_id_ || a.product_id_ != b.product_id_;
    }


    inline std::ostream& operator<<(std::ostream& out, const ProductOffer &o) {
        out << "{ p_id: " << o.product_id_ << ", m_id: " << o.market_id_ << ", p: " << o.price_
            << ", q: " << o.quantity_ << " }";
        return out;
    }


    /*
     * Contains the Traveling Purchaser Problem instance data.
     */
    struct Instance {
        string name_{};
        size_t dimension_{ 0 };
        vector<vector<int>> edge_weights_;
        vector<int> edge_weights_1d_;
        // nn_lists[i] - a list of neighbors of the market i sorted according
        // to edge weight
        vector<vector<uint32_t>> nn_lists_;
        bool is_symmetric_{ true };

        size_t product_count_{ 0 };
        vector<int> demands_;  // demands_[i] = demand for product i
        vector<uint32_t> needed_products_;  // A list of required products' ids
        // Product offers within market are sorted from the cheapest to the
        // most expensive
        vector<vector<ProductOffer>> market_offers_; // [i] = a list of offers at market i
        // market_product_offers_[i][j] = offer for prod. j at market i
        vector<vector<ProductOffer>> market_product_offers_;
        bool is_capacitated_{ false };
        int best_known_cost_{ 0 };  // From an exteral source


        int get_travel_cost(size_t market_a, size_t market_b) const noexcept;

        int calc_travel_cost(const vector<uint32_t> &route) const noexcept;

        std::vector<int> get_max_product_prices() const noexcept;
    };


    Instance load_from_file(const string path);


    /**
     * Returns true if route represents a valid TPP solution, based on the data
     * in instance.
     */
    bool is_solution_valid(const Instance& instance,
                           const vector<uint32_t>& route);


    /**
    * Returns cost of a TPP solution given in route and based on the data in
    * instance.
    */
    int calc_solution_cost(const Instance& instance,
                           const vector<uint32_t> &route);



    void run_tests();
}


#endif
