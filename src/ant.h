#ifndef ANT_H
#define ANT_H

#include "tpp_solution.h"


struct Ant {
    TPP::Solution solution_;
    // Parameters as in the article of B. Bontoux & D. Feillet:
    double independence_ = 1;
    double affinity_ = 1;
    double laziness_ = 1;
    double avidity_ = 1;
    double oversize_ = 0.1;
    size_t length_when_valid_ = 0;
    uint32_t id_{ 0 };


    Ant(const TPP::Instance &instance);

    void move_to(size_t market);

    int cost() const;

    bool operator<(const Ant &other) const noexcept { return cost() < other.cost(); }

    size_t get_position() const noexcept { return solution_.route_.back(); }

    /**
    * Returns a list of markets that are candidates for the next move. First it
    * tries to return only unvisited nearest neighbors but if at least 2 of them
    * are still remaining. Otherwise, it returns all the remaining unselected
    * markets.
    */
    const std::vector<uint32_t>& get_candidate_markets(size_t nn_count) noexcept;

    std::vector<uint32_t>& get_route() noexcept { return solution_.route_; }
};


#endif
