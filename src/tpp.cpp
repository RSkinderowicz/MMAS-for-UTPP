#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "tpp.h"
#include "logging.h"
#include "utils.h"


using namespace TPP;
using namespace std;


int TPP::Instance::get_travel_cost(size_t market_a,
                                   size_t market_b) const noexcept {
    // return edge_weights_.at(market_a).at(market_b);
    // return edge_weights_[market_a][market_b];
    return edge_weights_1d_.at(market_a * dimension_ + market_b);
}


pair<size_t, vector<int>> read_demand_section(ifstream &file) {
    vector<int> demands;
    size_t product_count{ 0 };

    LOG_SCOPE_F(INFO, "Reading demands...");

    string line;
    if (getline(file, line, '\n')) {
        auto n = stoi(line);
        assert(n > 0);
        product_count = static_cast<size_t>(n);

        for (auto i = 0u; i < product_count; ++i) {
            if (!getline(file, line, '\n')) {
                LOG_F(ERROR, "Lines missing in DEMAND_SECTION");
                abort();
            }
            auto iss = istringstream(line);
            size_t id{ 0 };
            size_t demand{ 0 };
            iss >> id >> demand;
            assert(id == i+1);
            demands.push_back(demand);
        }
    }
    LOG_F(INFO, "Total products: %zu", product_count);
    return make_pair(product_count, demands);
}


/*
 * Reads exactly market_count product offers from file.
 */
vector<vector<ProductOffer>>
read_offer_section(ifstream &file, size_t market_count) {
    vector<vector<ProductOffer>> market_offers;

    market_offers.resize(market_count);
    LOG_SCOPE_F(INFO, "read_offer_section");

    for (auto i = 0u; i < market_count; ++i) {
        string line;
        if (!getline(file, line, '\n')) {
            LOG_F(ERROR, "Too few lines in offer section");
            abort();
        }
        auto iss = istringstream(line);
        int market_id{ 0 };
        size_t offer_count{ 0 };

        iss >> market_id >> offer_count;
        assert(market_id == static_cast<int>(i + 1));

        auto &offers = market_offers.at(i);

        for (auto j = 0u; j < offer_count; ++j) {
            ProductOffer offer;
            iss >> offer.product_id_ >> offer.price_ >> offer.quantity_;
            --offer.product_id_;  // Keep product ids in range [0..dimension-1]
            offer.market_id_ = i;
            offers.emplace_back(offer);
            CHECK_F(offer.price_ >= 0, "Price should be >= 0");
            CHECK_F(offer.quantity_ > 0, "Quantity should be > 0");
        }
    }

    return market_offers;
}

/*
 * Only these types of edge weight type are supported:
 */
enum class EdgeWeightType {
    EUC_2D,
    EXPLICIT
};


enum class EdgeWeightFormat {
    UPPER_ROW
};


/*
 * Loads explicitly given edge weights matrix
 */
vector<vector<int>> read_edge_weights(ifstream &file, size_t dimension,
                                      EdgeWeightFormat edge_weight_format) {
    vector<vector<int>> edge_weights;

    LOG_SCOPE_F(INFO, "read_edge_weights");

    edge_weights.resize(dimension);

    if (edge_weight_format != EdgeWeightFormat::UPPER_ROW) {
        LOG_F(ERROR, "Unsupported edge weight format");
        abort();
    }

    // Read upper half of the weights section
    for (auto i = 1u; i < dimension; ++i) {
        string line;
        if (!getline(file, line, '\n')) {
            LOG_F(ERROR, "Too few lines in offer section");
            abort();
        }

        auto &weights = edge_weights.at(i - 1u);
        weights.resize(dimension);
        auto iss = istringstream(line);
        for (auto j = i; j < dimension; ++j) {
            iss >> weights.at(j);
        }
    }
    edge_weights.at(dimension-1).resize(dimension);

    // Fill in the lower left half
    for (auto i = 0u; i < dimension; ++i) {
        for (auto j = i + 1; j < dimension; ++j) {
            edge_weights.at(j).at(i) = edge_weights.at(i).at(j);
        }
    }
    return edge_weights;
}


vector<pair<int, int>> read_node_coords_section(ifstream &file,
                                                size_t dimension) {
    vector<pair<int, int>> coords;
    coords.reserve(dimension);

    for (auto i = 0u; i < dimension; i++) {
        string line;
        if (!getline(file, line, '\n')) {
            LOG_F(ERROR, "Too few lines in offer section");
            abort();
        }
        istringstream iss{line};
        size_t node_id;
        int x, y;
        iss >> node_id >> x >> y;

        assert(node_id == i+1);

        coords.push_back(make_pair(x, y));
    }
    assert(coords.size() == dimension);
    return coords;
}


/**
 * Calculates edge weights based on Euclidean distances.
 *
 * See: http://jriera.webs.ull.es/TPPLIB/Description.pdf
 */
vector<vector<int>> calc_edge_weight_matrix(const vector<pair<int, int>> &coords,
                                            EdgeWeightType edge_weight_type) {
    assert(edge_weight_type == EdgeWeightType::EUC_2D);

    vector<vector<int>> weights;

    const auto dimension = coords.size();
    assert(dimension > 1u);

    weights.resize(dimension);
    for (auto &vec : weights) {
        vec.resize(dimension);
    }

    for (auto i = 0u; i < dimension; i++) {
        for (auto j = 0u; j < i; ++j) {
            const auto &a = coords[i];
            const auto &b = coords[j];

            double xd = a.first - b.first;
            double yd = a.second - b.second;

            double co = sqrt(xd * xd + yd * yd);
            int weight = static_cast<int>(co);

            weights[i][j] = weights[j][i] = weight;
        }
    }
    return weights;
}


/**
 * Returns a list of nearest neighbors for each market (according to edge
 * weights). Each list has size equal to instance.dimension - 1.
 */
vector<vector<uint32_t>> calc_nearest_neighbors(const TPP::Instance &instance) {
    CHECK_F(instance.dimension_ > 0);
    vector<vector<uint32_t>> nn_lists(instance.dimension_);

    uint32_t from = 0;

    for (auto i = 0u; i < instance.dimension_; ++i) {
        auto &nn_list = nn_lists.at(i);
        nn_list.reserve(instance.dimension_ - 1);

        for (auto j = 0u; j < instance.dimension_; ++j) {
            if (j != i) {  // Do not add self to the nn list
                nn_list.push_back(j);
            }
        }
        auto cmp = [from=i, &instance]
                   (uint32_t m1, uint32_t m2) -> bool {
                       return instance.get_travel_cost(from, m1)
                            < instance.get_travel_cost(from, m2);
                   };
        sort(begin(nn_list), end(nn_list), cmp);
        // Now we have the closest markets first in nn_list
    }
    return nn_lists;
}


/*
 * Loads TPP instance from the file in the format specifed in:
 * http://jriera.webs.ull.es/TPPLIB/TPPLIBFormat.htm
 */
Instance TPP::load_from_file(const string path) {
    Instance instance;
    LOG_SCOPE_F(INFO, "Loading TPP instance: %s", path.c_str());

    EdgeWeightFormat edge_weight_format;
    EdgeWeightType edge_weight_type{ EdgeWeightType::EUC_2D };

    ifstream file{ path };

    if (file.is_open()) {
        LOG_F(INFO, "File opened");

        string line;

        while (getline(file, line, '\n')) {
            //LOG_F(INFO, "%s", line.c_str());

            auto prefix = string(line);
            auto suffix = string();

            const auto pos = line.find(':');
            const bool has_colon = pos != string::npos;
            if (has_colon) {
                prefix = line.substr(0, pos);
                suffix = line.substr(pos + 1);
            }

            trim(prefix);
            trim(suffix);

            //LOG_F(INFO, "[%s] : [%s]", prefix.c_str(), suffix.c_str());

            if (starts_with(prefix, "NAME")) {
                instance.name_ = suffix;
            } else if (starts_with(prefix, "TYPE")) {
                assert(suffix == "TPP");
            } else if (starts_with(prefix, "COMMENT")) {
                /* ignore */
                LOG_F(INFO, "Instance comment: %s", suffix.c_str());
            } else if (starts_with(prefix, "DIMENSION")) {
                auto n = stoi(suffix);
                assert(n >= 2);

                instance.dimension_ = static_cast<size_t>(n);
            } else if (starts_with(prefix, "EDGE_WEIGHT_TYPE")) {
                if (suffix == "EXPLICIT") {
                    edge_weight_type = EdgeWeightType::EXPLICIT;
                } else if (suffix == "EUC_2D") {
                    edge_weight_type = EdgeWeightType::EUC_2D;
                } else {
                    LOG_F(ERROR, "Unknown edge weight type: %s", suffix.c_str());
                    abort();
                }
            } else if (starts_with(prefix, "EDGE_WEIGHT_FORMAT")) {
                edge_weight_format = EdgeWeightFormat::UPPER_ROW;
                instance.is_symmetric_ = true;
            } else if (starts_with(prefix, "DISPLAY_DATA_TYPE")) {
                // ignore
            } else if (starts_with(prefix, "DEMAND_SECTION")) {
                auto res = read_demand_section(file);
                instance.product_count_ = res.first;
                instance.demands_ = res.second;
                for (auto p = 0u; p < instance.product_count_; ++p) {
                    auto demand = instance.demands_.at(p);
                    if (demand > 0) {
                        instance.needed_products_.push_back(p);
                    }
                    // We assume that if there is at least one product for
                    // which demand is > 1 then the TPP instance is capacitated
                    if (demand > 1) {
                        instance.is_capacitated_ = true;
                    }
                }
            } else if (starts_with(prefix, "OFFER_SECTION")) {
                auto res = read_offer_section(file, instance.dimension_);
                // Sort offers by price, i.e. from the lowest to the highest
                for (auto &offers : res) {
                    sort(begin(offers), end(offers), has_lower_price);
                }
                instance.market_offers_ = res;

                auto &mpo = instance.market_product_offers_;
                mpo.resize(instance.dimension_);

                for (auto m = 0u; m < instance.dimension_; ++m) {
                    auto &mpo_row = mpo.at(m);
                    mpo_row.resize(instance.product_count_);
                    for (const auto &offer : instance.market_offers_.at(m)) {
                        mpo_row.at(offer.product_id_) = offer;
                    }
                }
            } else if (starts_with(prefix, "EDGE_WEIGHT_SECTION")) {
                assert(edge_weight_type == EdgeWeightType::EXPLICIT);

                auto res = read_edge_weights(file, instance.dimension_,
                                             edge_weight_format);
                instance.edge_weights_ = res;
            } else if (starts_with(prefix, "EOF")) {
                // Ignore
            } else if (starts_with(prefix, "EDGE_DATA_FORMAT")) {
                LOG_F(INFO, "Ignoring EDGE_DATA_FORMAT: %s", suffix.c_str());
            } else if (starts_with(prefix, "NODE_COORD_TYPE")) {
                assert(suffix == "TWOD_COORDS");
            } else if (starts_with(prefix, "NODE_COORD_SECTION")) {
                auto coords = read_node_coords_section(file, instance.dimension_);
                instance.edge_weights_ = calc_edge_weight_matrix(coords,
                        edge_weight_type);
            } else {
                LOG_F(ERROR, "Unknown section: %s", prefix.c_str());
                break ;
            }
        }
        file.close();
    }
    if (instance.name_.empty()) {
        // A naive extraction of filename
        auto it = path.find_last_of("/");
        const auto filename = (it == string::npos) ? path
                                                   : path.substr(it + 1);
        if (filename.find(".tpp") != string::npos) {
            instance.name_ = filename.substr(0, filename.size() - 4);
        } else {
            instance.name_ = filename;
        }
    }

    instance.edge_weights_1d_.resize(instance.dimension_ * instance.dimension_);
    for (auto i = 0u; i < instance.dimension_; ++i) {
        for (auto j = 0u; j < instance.dimension_; ++j) {
            instance.edge_weights_1d_.at(i * instance.dimension_ + j) = instance.edge_weights_.at(i).at(j);
        }
    }

    instance.nn_lists_ = calc_nearest_neighbors(instance);

    return instance;
}


/**
 * Returns true if route represents a valid TPP solution, based on the data in
 * instance.
*/
bool TPP::is_solution_valid(const Instance& instance,
                            const vector<uint32_t> &route) {
    if (route.size() > instance.dimension_) {
        LOG_F(INFO, "route too long: %zu", route.size());
        return false;
    }

    if (route.at(0) != 0) {
        LOG_F(INFO, "First node should be 0 (depot), not : %zu", route[0]);
        return false;
    }

    // A sum of product quantities available at visitied markets
    vector<int> product_quantities;
    product_quantities.resize(instance.product_count_);

    for (const auto node : route) {
        for (const auto &offer : instance.market_offers_.at(node)) {
            auto id = offer.product_id_;
            product_quantities.at(id) += offer.quantity_;
        }
    }
    for (auto i = 0ul, n = instance.demands_.size(); i < n; ++i) {
        if (product_quantities.at(i) < instance.demands_[i]) {
            LOG_F(INFO, "Product %zu quantity %d too small, required: %d", i,
                  product_quantities[i], instance.demands_[i]);
            return false;
        }
    }
    return true;
}


/**
 * Returns cost of a TPP solution given in route and based on the data in
 * instance.
 *
 * This assumes that the instance is uncapacitated, i.e. a demand for each
 * product is at most 1.
 * The calculations are more efficient than for the capacitated version.
 *
 * The complexity  O(MK) where K is the number of markets and L the number of
 * products.
*/
int calc_solution_cost_uncapacitated(const Instance& instance,
                                   const vector<uint32_t> &route) {
    if (route.size() > instance.dimension_) {
        LOG_F(INFO, "route too long: %zu", route.size());
        return -1;
    }

    if (route.at(0) != 0) {
        LOG_F(INFO, "First node should be 0 (depot), not : %zu", route[0]);
        return -1;
    }
    CHECK_F(instance.is_capacitated_ == false,
            "Uncapacitated TPP instance required");

    // Lowest product prices available at visitied markets
    vector<int> product_offers;
    product_offers.resize(instance.product_count_,
                          numeric_limits<int>::max());

    int total_distance = 0;

    auto prev = route.back();
    for (const auto node : route) {
        total_distance += instance.get_travel_cost(prev, node);

        for (const auto &offer : instance.market_offers_.at(node)) {
            const auto p = offer.product_id_;
            if (instance.demands_.at(p) > 0) {
                product_offers[p] = min(product_offers.at(p), offer.price_);
            }
        }
        prev = node;
    }
    int purchase_cost = 0;
    for (auto el : product_offers) {
        purchase_cost += el;
    }
    return total_distance + purchase_cost;
}


/**
 * Returns cost of a TPP solution given in route and based on the data in
 * instance.
 *
 * The complexity  O(MK log K) where K is the number of markets and L the number of
 * products.
*/
int calc_solution_cost_capacitated(const Instance& instance,
                                 const vector<uint32_t> &route) {
    if (route.size() > instance.dimension_) {
        LOG_F(INFO, "route too long: %zu", route.size());
        return -1;
    }

    if (route.at(0) != 0) {
        LOG_F(INFO, "First node should be 0 (depot), not : %zu", route[0]);
        return -1;
    }

    // A sum of product quantities available at visitied markets
    vector<vector<ProductOffer>> product_offers;
    product_offers.resize(instance.product_count_);

    int total_distance = 0;

    auto prev = route.back();
    for (const auto market : route) {
        total_distance += instance.get_travel_cost(prev, market);

        for (const auto &offer : instance.market_offers_.at(market)) {
            auto id = offer.product_id_;
            if (instance.demands_.at(id) > 0) {
                product_offers.at(id).push_back(offer);
            }
        }
        prev = market;
    }
    for (auto &offers : product_offers) {
        sort(begin(offers), end(offers),
             [](const auto &o1, const auto &o2) {
                return o1.price_ < o2.price_;
             });
    }
    int purchase_cost = 0;
    for (auto i = 0ul, n = instance.demands_.size(); i < n; ++i) {
        int needed = instance.demands_[i];
        for (const auto &offer : product_offers.at(i)) {
            const auto bought = min(needed, offer.quantity_);
            needed -= bought;
            purchase_cost += offer.price_ * bought;
            if (needed == 0) {
                break ;
            }
        }
    }
    return total_distance + purchase_cost;
}


/**
 * Returns cost of a TPP solution given in route and based on the data in
 * instance.
*/
int TPP::calc_solution_cost(const Instance& instance,
                            const vector<uint32_t> &route) {
    if (instance.is_capacitated_) {
        return calc_solution_cost_capacitated(instance, route);
    }
    return calc_solution_cost_uncapacitated(instance, route);
}


int TPP::Instance::calc_travel_cost(const vector<uint32_t> &route) const noexcept {
    int cost = 0;

    auto prev = route.back();
    for (const auto node : route) {
        cost += get_travel_cost(prev, node);
        prev = node;
    }
    return cost;
}


std::vector<int> TPP::Instance::get_max_product_prices() const noexcept {
    vector<int> product_prices(product_count_, 0);

    for (const auto &offers : market_offers_) {
        for (const auto &offer : offers) {
            auto &price = product_prices.at(offer.product_id_);
            price = max(price, offer.price_);
        }
    }
    return product_prices;
}



void test_is_solution_valid() {
    LOG_SCOPE_F(INFO, "test_is_solution_valid");
    vector<int> weights{ 0, 1, 1, 1,
                                 1, 0, 1, 1,
                                 1, 1, 0, 1,
                                 1, 1, 1, 0 };

    vector<ProductOffer> m0{},

                         m1{ {/*cost*/1, /*quantity*/1, /*id*/0 },
                             {/*cost*/1, /*quantity*/1, /*id*/1 } },

                         m2{ {/*cost*/2, /*quantity*/1, /*id*/1 },
                             {/*cost*/1, /*quantity*/1, /*id*/2 } },

                         m3{ {/*cost*/2, /*quantity*/1, /*id*/0 },
                             {/*cost*/2, /*quantity*/1, /*id*/1 } };

    vector<vector<ProductOffer>> offers{ m0, m1, m2, m3 };

    vector<int> demands{ 2, 1, 1 };

    Instance instance;
    instance.dimension_ = 4;;
    instance.edge_weights_1d_ = weights;
    instance.is_symmetric_ = true;
    instance.product_count_ = 3;
    instance.demands_ = demands;
    instance.market_offers_ = offers;

    vector<uint32_t> sol1{ 0, 1, 2, 3 };

    CHECK_F(TPP::is_solution_valid(instance, sol1), "Expected valid solution");

    vector<uint32_t> sol2{ 1, 2, 3 };
    CHECK_F(!TPP::is_solution_valid(instance, sol2), "Expected invalid solution");

    vector<uint32_t> sol3{ 0, 1, 3 };
    CHECK_F(!TPP::is_solution_valid(instance, sol3), "Expected invalid solution");

    vector<uint32_t> sol4{ 0, 1, 2 };
    CHECK_F(!TPP::is_solution_valid(instance, sol4), "Expected invalid solution");
}


void test_calc_solution_cost() {
    LOG_SCOPE_F(INFO, "test_calc_solution_cost");
    vector<int> weights{ 0, 1, 1, 1,
                                 1, 0, 1, 1,
                                 1, 1, 0, 1,
                                 1, 1, 1, 0 };

    vector<ProductOffer> m0{},

                         m1{ {/*cost*/1, /*quantity*/2, /*id*/0 },
                             {/*cost*/2, /*quantity*/2, /*id*/1 } },

                         m2{ {/*cost*/2, /*quantity*/2, /*id*/1 },
                             {/*cost*/1, /*quantity*/2, /*id*/2 } },

                         m3{ {/*cost*/2, /*quantity*/2, /*id*/0 },
                             {/*cost*/1, /*quantity*/2, /*id*/1 } };

    vector<vector<ProductOffer>> offers{ m0, m1, m2, m3 };

    vector<int> demands1{ 1, 1, 1 };

    Instance instance;
    instance.dimension_ = 4;
    instance.edge_weights_1d_ = weights;
    instance.is_symmetric_ = true;
    instance.product_count_ = 3;
    instance.demands_ = demands1;
    instance.market_offers_ = offers;

    vector<uint32_t> sol1{ 0, 1, 2, 3 };

    auto cost1 = TPP::calc_solution_cost(instance, sol1);
    CHECK_F(cost1 == 4 + 3, "Expected cost: %d got: %d", 4 + 3, cost1);

    auto cost1a = calc_solution_cost_uncapacitated(instance, sol1);
    auto cost1b = calc_solution_cost_capacitated(instance, sol1);
    CHECK_F(cost1a == cost1b, "Uncapcacited and capacitated costs should be equal");

    vector<int> demands2{ 3, 1, 1 };
    instance.demands_ = demands2;
    instance.is_capacitated_ = true;
    auto cost2 = TPP::calc_solution_cost(instance, sol1);
    CHECK_F(cost2 == 4 + (2*1 + 1*2) + 1*1 + 1*1,
            "Expected cost: %d got: %d", 10, cost2);
}


void TPP::run_tests() {
    LOG_F(INFO, "Running tests");
    test_is_solution_valid();
    test_calc_solution_cost();
}
