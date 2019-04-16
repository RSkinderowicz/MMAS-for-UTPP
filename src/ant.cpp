#include "ant.h"
#include "rand.h"
#include "logging.h"

using namespace std;


Ant::Ant(const TPP::Instance &instance)
    : solution_(instance) {
    affinity_ = 3; // + get_random_value() * 8;
    laziness_ = 2; // + get_random_value() * 8;
    avidity_ =  2; // + get_random_value() * 8;
    oversize_ = 0.0 + get_random_value() * 0.1;
}


void Ant::move_to(size_t market) {
    CHECK_F(market != 0, "Cannot move to depot");
    solution_.push_back_market(market);

    if (solution_.is_valid()) {
        length_when_valid_ = solution_.route_.size();
    }
}


int Ant::cost() const {
    return solution_.cost_;
}


/*
 * Returns a list of markets that are candidates for the next move. First it
 * tries to return only unvisited nearest neighbors but if at least 2 of them
 * are still remaining. Otherwise, it returns all the remaining unselected
 * markets.
 */
const std::vector<uint32_t>&
Ant::get_candidate_markets(size_t nn_count) noexcept {
    const auto current_market = solution_.route_.back();

    static vector<uint32_t> cand;
    cand.reserve(nn_count);
    cand.clear();
    const auto &all_nn = solution_.instance_.nn_lists_.at(current_market);
    for (auto i = 0u; i < nn_count; ++i) {
        const auto market = all_nn.at(i);
        if (market != 0  // We do not want to add depot
            && !solution_.is_market_used(market)) {
            cand.push_back(market);
        }
    }
    // cand should contain at least 2 markets to allow some choice
    if (cand.size() > 1) {
        return cand;
    }
    const auto unselected = solution_.get_unselected_markets();
    cand.resize(unselected.size());
    copy(begin(unselected), end(unselected), begin(cand));
    return cand;
}
