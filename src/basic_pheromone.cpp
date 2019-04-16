#include "basic_pheromone.h"


using namespace std;


BasicPheromone::BasicPheromone(uint32_t size, bool is_symmetric,
                               double min_value, double max_value)
    : trails_(size),
      is_symmetric_(is_symmetric),
      min_value_(min_value),
      max_value_(max_value) {

    for (auto &vec : trails_) {
        vec.resize(size, max_value);
    }
}


double BasicPheromone::get_trail(uint32_t a, uint32_t b) const noexcept {
    return trails_.at(a).at(b);
}


void BasicPheromone::increase(uint32_t from, uint32_t to, double delta) {

    auto &val = trails_.at(from).at(to);
    val = min(max_value_, val + delta);
    if (is_symmetric_) {
        trails_[to][from] = val;
    }
}


void BasicPheromone::evaporate(double evaporation_ratio) {
    for (auto &vec : trails_) {
        for (auto &trail : vec) {
            trail = max(min_value_, trail * evaporation_ratio);
        }
    }
}


void BasicPheromone::set_all_trails(double value) {
    for (auto &vec : trails_) {
        fill(begin(vec), end(vec), value);
    }
}


void BasicPheromone::set_trail_limits(double min_value, double max_value) {
    min_value_ = min_value;
    max_value_ = max_value;
}
