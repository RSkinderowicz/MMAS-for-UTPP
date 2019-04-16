#include <numeric>
#include <algorithm>
#include <iterator>
#include <array>

#include "three_opt.h"
#include "rand.h"
#include "logging.h"
#include "utils.h"
#include "stopcondition.h"

using namespace std;


/*
 * A simple impl. of a "wrapped" iterator that starts from the begin() if
 * reached end()
 *
 * It is used in the 3-opt implementation during route's segments
 * modifications/movements.
 */
template<class Type,
         class UnqualifiedType = std::remove_cv_t<Type>>
class WrappedIterator
    : public std::iterator<std::bidirectional_iterator_tag,
                           UnqualifiedType,
                           std::ptrdiff_t,
                           Type*,
                           Type&>
{

    using iter_t = typename std::vector<UnqualifiedType>::iterator;
    using vector_t = std::vector<UnqualifiedType>;
    vector_t *vec_{ nullptr };
    iter_t anchor_, it_;
    bool wrapped_{ false };
    bool at_the_end_{ false };

public:

    explicit WrappedIterator(vector_t &vec, iter_t anchor, iter_t it, bool is_end)
        : vec_(&vec),
          anchor_(anchor),
          it_(it),
          at_the_end_(is_end)
    {
    }


    explicit WrappedIterator(vector_t &vec,
                             int start_index,
                             int shift_from_start = 0)
        : vec_(&vec)
    {
        const int len = static_cast<int>(vec.size());

        CHECK_F(start_index < len, "start_index < vec.size()");
        CHECK_F(shift_from_start <= len, "%d <= %d", shift_from_start, len);

        anchor_ = vec.begin() + start_index;
        it_ = vec.begin() + (start_index + shift_from_start) % len;
        if (shift_from_start == len) {
            at_the_end_ = true;
        }
    }


    explicit WrappedIterator(vector_t &vec,
                             int start_index,
                             bool is_end)
        : vec_(&vec),
          at_the_end_(is_end)
    {
        const int len = static_cast<int>(vec.size());

        CHECK_F(start_index < len, "start_index < vec.size()");

        it_ = anchor_ = vec.begin() + start_index;
    }


    WrappedIterator& operator++ () // Pre-increment
    {
        if (!wrapped_) {
            if (++it_ == vec_->end()) {
                it_ = vec_->begin();
                wrapped_ = true;
            }
        } else if (it_ != anchor_) {
            ++it_;
        } else {
            at_the_end_ = true;
        }
        return *this;
    }


    WrappedIterator& operator-- () // Pre-decrement
    {
        if (it_ == vec_->begin()) {
            it_ = vec_->end();
        }
        --it_;
        at_the_end_ = false;
        return *this;
    }

    WrappedIterator operator-- (int) // Post-increment
    {
        WrappedIterator tmp(*this);
        this->operator--();
        return tmp;
    }

    // two-way comparison: v.begin() == v.cbegin() and vice versa
    template<class OtherType>
    bool operator == (const WrappedIterator<OtherType>& rhs) const
    {
        return it_ == rhs.it_ && at_the_end_ == rhs.at_the_end_;
    }

    template<class OtherType>
    bool operator != (const WrappedIterator<OtherType>& rhs) const
    {
        return it_ != rhs.it_ || at_the_end_ != rhs.at_the_end_;
    }

    Type& operator* () const { return *it_; }

    Type& operator-> () const { return *it_; }

    // One way conversion: iterator -> const_iterator
    operator WrappedIterator<const Type>() const
    {
        return WrappedIterator<const Type>(*this);
    }
};


/*
 * Segment corresponds to a fragment (segment) of a route (vector), i.e. a
 * sequence of consecutive indices of the vector.
 */
struct Segment {
    uint32_t first_{ 0 }; // Index of the first element belonging to a segment
    uint32_t last_{ 0 };  // Index of the last element of the segment
    uint32_t len_{ 0 };   // Length of a whole route (vector)
    uint32_t id_{ 0 };    // We can give each segment an id / index
    bool is_reversed_{ false }; // This is used to denote that the order of elements
                                // within segment should be reversed

    /* Returns length of a segment */
    uint32_t size() const {
        if (is_reversed_) {
            return get_reversed().size();
        }
        if (first_ <= last_) {
            return last_ - first_ + 1;
        }
        return len_ - first_ + last_ + 1;
    }

    void reverse() {
        swap(first_, last_);
        is_reversed_ = !is_reversed_;
    }

    Segment get_reversed() const {
        return Segment{ last_, first_, len_, id_, !is_reversed_ };
    }

    int32_t first() const { return static_cast<int32_t>(first_); }
    int32_t last() const { return static_cast<int32_t>(last_); }
    int32_t isize() const { return static_cast<int32_t>(size()); }
};


bool contains_edge(const vector<uint32_t>& vec, uint32_t a, uint32_t b) {
    if (a > b) {
        swap(a, b);
    }
    auto prev = vec.back();
    for (auto node : vec) {
        auto c = prev;
        auto d = node;
        if (c > d) {
            swap(c, d);
        }
        if (c == a && d == b) {
            return true;
        }
        prev = node;
    }

    return false;
}


void perform_2_opt_move(vector<uint32_t> &route, uint32_t i, uint32_t j) {
    using iter_t = WrappedIterator<uint32_t>;

    const auto len = route.size();
    if (i > j) {
        swap(i, j);
    }
    // Now we have ..., i), (i+1, ..., j), (j+1, ...
    uint32_t i_1 = (i + 1) % len;
    uint32_t j_1 = (j + 1) % len;
    Segment s1 { i_1, j };
    Segment s2 { j_1, i };
    if (s1.size() < s2.size()) {
        reverse(begin(route) + s1.first_,
                begin(route) + s1.first_ + s1.size());
    } else {
        reverse(iter_t(route, static_cast<int>(s2.first_)),
                iter_t(route, static_cast<int>(s2.first_), s2.isize()));
    }
}


/**
 * Performs route modifications required by a 3-opt move, i.e. segments
 * reversals and swaps.
 *
 * The longest of the { s0, s1, s2 } segments is not modified, instead the
 * remaining segments are reversed if necessary.
 *
 * This works only for the symmetric version of the TSP.
 */
void perform_3_opt_move(vector<uint32_t> &route, Segment s0, Segment s1, Segment s2) {
    using iter_t = WrappedIterator<uint32_t>;

    // Sort segments so that the longest one is the first - it
    // will be kept without changes
    if (s0.size() < s1.size()) {
        swap(s0, s1);
    }
    if (s0.size() < s2.size()) {
        swap(s0, s2);
    }
    if (s1.size() < s2.size()) {
        swap(s1, s2);
    }
    CHECK_F((s0.size() >= s1.size()) && (s1.size() >= s2.size()),
            "Segments should be sorted");

    bool swap_needed = false;  // Do we need to swap shorter segments?

    // We do not want to touch the longest (first) segment so
    // if it is reversed we reverse the other two instead and do a
    // swap
    if (s0.is_reversed_) {
        s1.reverse();
        s2.reverse();
        swap_needed = true;
    }
    // segment[0] is OK, so touch only segment[1] and [2]
    if (s1.is_reversed_) {
        s1.reverse();

        reverse(iter_t(route, s1.first_),
                iter_t(route, s1.first_, s1.isize()));
    }
    if (s2.is_reversed_) {
        s2.reverse();

        reverse(iter_t(route, s2.first_),
                iter_t(route, s2.first_, s2.isize()));
    }
    if (swap_needed) {
        auto beg = route.begin();

        // Now perform the swap of s1 and s2, we use std::rotate
        if (s1.id_ == 2 && s2.id_ == 1) { // 0 2 1, easy case
            rotate(beg + s2.first(), beg + s1.first(),
                   beg + s1.last() + 1);
        } else if (s1.id_ == 1 && s2.id_ == 2) { // 0 1 2, easy case
            rotate(beg + s1.first(), beg + s2.first(),
                   beg + s2.last() + 1);
        } else {
            auto left = 0;
            auto middle = 0;
            auto right = s1.isize() + s2.isize();

            if ( (s1.id_ == 0 && s2.id_ == 2) // 1 0 2
              || (s1.id_ == 1 && s2.id_ == 0) ) {  // 2 1 0
                left = s2.first();
                middle = s2.size();
            } else if ( (s1.id_ == 2 && s2.id_ == 0) // 1 2 0
                     || (s1.id_ == 0 && s2.id_ == 1) ) {  // 2 0 1
                left = s1.first();
                middle = s1.size();
            }
            rotate(iter_t(route, left),
                   iter_t(route, left, middle),
                   iter_t(route, left, right));
        }
    }
}


/*
 * Impl. of the 3-opt heuristic. Tries to change the order of markets in
 * solution to shorten the travel distance.
 *
 * The solution's route is modified only if a better order was found.
 *
 * Returns improvement over the previous route length (travel distance).
*/
int three_opt(const TPP::Instance &instance, TPP::Solution &sol,
              bool use_dont_look_bits) {
    LOG_SCOPE_F(INFO, "three_opt");

    CHECK_F(instance.is_symmetric_, "Symmetric instance expected!");

    const auto len = static_cast<uint32_t>(sol.route_.size());
    auto &route = sol.route_;
    const auto old_travel_cost = instance.calc_travel_cost(route);

    vector<uint8_t> dont_look_bits(instance.dimension_, false);

    auto found_improvement = false;

    do {
        found_improvement = false;
        for (auto i = 0u; i < len - 2 && !found_improvement; ++i) {
            if (dont_look_bits[route[i]]) {
                continue ;  // Do not check, it probably won't find an
                            // improvement
            }
            for (auto j = i + 1; j < len - 1 && !found_improvement; ++j) {
                for (auto k = j + 1; k < len && !found_improvement; ++k) {

                    const auto k_1 = (k + 1) % len;
                    // For convenience
                    const auto _i = route[i];
                    const auto _i_1 = route[i + 1];
                    const auto _j = route[j];
                    const auto _j_1 = route[j + 1];
                    const auto _k = route[k];
                    const auto _k_1 = route[k_1];

                    const auto curr = instance.get_travel_cost(_i, _i_1)
                                    + instance.get_travel_cost(_j, _j_1)
                                    + instance.get_travel_cost(_k, _k_1);

                    // 4 sets of possible new edges to check
                    const array<pair<uint32_t, uint32_t>, 4 * 3> pairs{{
                        { _j, _i }, { _k_1, _j_1 }, { _k, _i_1 },
                        { _j, _k_1 }, { _i, _j_1 }, { _k, _i_1 },
                        { _j, _k_1 }, { _i, _k }, { _j_1, _i_1 },
                        { _j, _k }, { _j_1, _i }, { _k_1, _i_1 }
                    }};

                    // Which segments do we need to reverse in order to transform
                    // route so that the new edges are created properly
                    const array<bool, 4 * 3> segment_reversals{{
                        false, true, true,
                        true, true, true,
                        true, true, false,
                        true, false, true
                    }};

                    Segment seg[3] = {
                        { k_1, i, len, 0 },
                        { i + 1, j, len, 1 },
                        { j + 1, k, len, 2 }
                    };
                    for (auto l = 0u; l < 4 * 3 && !found_improvement; l += 3) {
                        const auto p1 = pairs[l + 0];
                        const auto p2 = pairs[l + 1];
                        const auto p3 = pairs[l + 2];

                        const auto cost = instance.get_travel_cost(p1.first, p1.second)
                                        + instance.get_travel_cost(p2.first, p2.second)
                                        + instance.get_travel_cost(p3.first, p3.second);

                        if (cost < curr) {
                            found_improvement = true;

                            if (segment_reversals[l + 0]) {
                                seg[0].reverse();
                            }
                            if (segment_reversals[l + 1]) {
                                seg[1].reverse();
                            }
                            if (segment_reversals[l + 2]) {
                                seg[2].reverse();
                            }
                            dont_look_bits[p1.first] = false;
                            dont_look_bits[p1.second] = false;

                            dont_look_bits[p2.first] = false;
                            dont_look_bits[p2.second] = false;

                            dont_look_bits[p3.first] = false;
                            dont_look_bits[p3.second] = false;
                        }
                    }
                    if (found_improvement) {
                        perform_3_opt_move(route, seg[0], seg[1], seg[2]);
                    }
                }
            }
            if (!found_improvement && use_dont_look_bits) {
                dont_look_bits[route[i]] = true;
            }
        }
    } while(found_improvement);

    // It may happen that depot is moved to another position
    auto depot_pos = find(begin(route), end(route), 0);
    if (depot_pos != route.begin()) {
        rotate(begin(route), depot_pos, end(route));
    }
    const auto new_travel_cost = instance.calc_travel_cost(route);
    const auto delta = new_travel_cost - old_travel_cost;
    CHECK_F(delta <= 0, "Travel cost should not be grater after 3-opt");
    sol.cost_ += delta;
    LOG_F(INFO, "3-opt improvement: %d", -delta);
    return delta;
}


/*
 * Impl. of the 3-opt heuristic. Tries to change the order of markets in
 * solution to shorten the travel distance.
 *
 * During the search for an improvement only edges connecting nn_count nearest
 * neighbors are taken into account to cut the overall search time.
 *
 * The solution's route is modified only if a better order was found.
 *
 * Function returns improvement over the previous route length (travel
 * distance).
*/
int three_opt_nn(const TPP::Instance &instance, TPP::Solution &sol,
                 bool use_dont_look_bits, size_t nn_count) {
    LOG_SCOPE_F(INFO, "three_opt_nn");

    CHECK_F(instance.is_symmetric_, "Symmetric instance expected!");

    const auto len = static_cast<uint32_t>(sol.route_.size());
    auto &route = sol.route_;
    const auto old_travel_cost = instance.calc_travel_cost(route);

    vector<uint8_t> dont_look_bits(instance.dimension_, false);
    vector<uint32_t> pos_in_route(instance.dimension_);

    auto found_improvement = false;

    do {
        found_improvement = false;

        fill_n(pos_in_route.begin(), instance.dimension_, len);
        for (auto i = 0u; i < route.size(); ++i) {
            pos_in_route[ route[i] ] = i;
        }

        for (auto i = 0u; i < len && !found_improvement; ++i) {
            if (dont_look_bits[route[i]]) {
                continue ;  // Do not check, it probably won't find an
                            // improvement
            }
            const auto at_i = route[i];
            const auto &i_nn_list = instance.nn_lists_.at(at_i);
            const auto i_nn_count = min(nn_count, i_nn_list.size());

            for (auto i_nn_idx = 0u; i_nn_idx < i_nn_count && !found_improvement; ++i_nn_idx) {
                const auto at_j = i_nn_list[i_nn_idx];
                const auto j = pos_in_route[at_j];

                if (j == len) {  // Not in route
                    continue ;
                }

                // Check for 2-opt move
                auto i_1 = (i + 1) % len;
                auto j_1 = (j + 1) % len;
                auto at_i_1 = route[i_1];
                auto at_j_1 = route[j_1];
                const auto change_2opt = instance.get_travel_cost(at_i, at_i_1)
                                       + instance.get_travel_cost(at_j, at_j_1)
                                       - instance.get_travel_cost(at_i, at_j)
                                       - instance.get_travel_cost(at_i_1, at_j_1);
                if (change_2opt > 0) {
                    const auto cost_before = instance.calc_travel_cost(route);
                    perform_2_opt_move(route, i, j);
                    const auto cost_after = instance.calc_travel_cost(route);
                    CHECK_F(cost_after < cost_before, "Improvement expected!");
                    CHECK_F(change_2opt + cost_after == cost_before, "Change should be equal to predicted");

                    found_improvement = true;

                    dont_look_bits[at_i] = false;
                    dont_look_bits[at_i_1] = false;

                    dont_look_bits[at_j] = false;
                    dont_look_bits[at_j_1] = false;

                    continue ;
                }

                const auto &j_nn_list = instance.nn_lists_.at(at_j);
                const auto j_nn_count = min(nn_count, j_nn_list.size());

                CHECK_F(at_i != at_j, "These two should be different");

                for (auto j_nn_idx = 0u; j_nn_idx < j_nn_count && !found_improvement; ++j_nn_idx) {
                    const auto at_k = j_nn_list[j_nn_idx];
                    const auto k = pos_in_route[at_k];

                    if (k == len || k == i) {  // Unlikely but possible, we want at_i != at_j != at_k
                        continue ;
                    }
                    //LOG_F(INFO, "i, j, k = %u, %zu, %zu", i, j, k);
                    //LOG_F(INFO, "at_i, at_j, at_k = %zu, %u, %u", at_i, at_j, at_k);

                    uint32_t x = i;
                    uint32_t y = j;
                    uint32_t z = k;
                    uint32_t at_x = at_i;
                    uint32_t at_y = at_j;
                    uint32_t at_z = at_k;

                    // Sort (x, y, z)
                    if (x > y) { swap(x, y); swap(at_x, at_y); }
                    if (x > z) { swap(x, z); swap(at_x, at_z); }
                    if (y > z) { swap(y, z); swap(at_y, at_z); }

                    auto x_1 = (x + 1) % len;
                    auto y_1 = (y + 1) % len;
                    auto z_1 = (z + 1) % len;

                    //LOG_F(INFO, "x, y, z = %zu, %zu, %zu", x, y, z);
                    //LOG_F(INFO, "at_x, at_y, at_z = %zu, %zu, %zu", at_x, at_y, at_z);

                    auto at_x_1 = route[x_1];
                    auto at_y_1 = route[y_1];
                    auto at_z_1 = route[z_1];

                    const auto curr = instance.get_travel_cost(at_x, at_x_1)
                                    + instance.get_travel_cost(at_y, at_y_1)
                                    + instance.get_travel_cost(at_z, at_z_1);

                    // 4 sets of possible new edges to check
                    const array<pair<uint32_t, uint32_t>, 4 * 3> pairs{{
                        { at_y, at_x }, { at_z_1, at_y_1 }, { at_z, at_x_1 },
                        { at_y, at_z_1 }, { at_x, at_y_1 }, { at_z, at_x_1 },
                        { at_y, at_z_1 }, { at_x, at_z }, { at_y_1, at_x_1 },
                        { at_y, at_z }, { at_y_1, at_x }, { at_z_1, at_x_1 }
                    }};

                    // Which segments do we need to reverse in order to transform
                    // route so that the new edges are created properly
                    const array<bool, 4 * 3> segment_reversals{{
                        false, true, true,
                        true, true, true,
                        true, true, false,
                        true, false, true
                    }};

                    Segment seg[3] = {
                        { z_1, x, len, 0 },
                        { x_1, y, len, 1 },
                        { y_1, z, len, 2 }
                    };

                    auto cost_change = 0;

                    for (auto l = 0u; l < 4 * 3 && !found_improvement; l += 3) {
                        const auto p1 = pairs[l + 0];
                        const auto p2 = pairs[l + 1];
                        const auto p3 = pairs[l + 2];

                        const auto cost = instance.get_travel_cost(p1.first, p1.second)
                                        + instance.get_travel_cost(p2.first, p2.second)
                                        + instance.get_travel_cost(p3.first, p3.second);

                        if (cost < curr) {
                            found_improvement = true;
                            cost_change = cost - curr;

                            if (segment_reversals[l + 0]) {
                                seg[0].reverse();
                            }
                            if (segment_reversals[l + 1]) {
                                seg[1].reverse();
                            }
                            if (segment_reversals[l + 2]) {
                                seg[2].reverse();
                            }
                            dont_look_bits[p1.first] = false;
                            dont_look_bits[p1.second] = false;

                            dont_look_bits[p2.first] = false;
                            dont_look_bits[p2.second] = false;

                            dont_look_bits[p3.first] = false;
                            dont_look_bits[p3.second] = false;
                        }
                    }
                    if (found_improvement) {
                        //const auto cost_before = instance.calc_travel_cost(route);
                        perform_3_opt_move(route, seg[0], seg[1], seg[2]);
                        //const auto cost_after = instance.calc_travel_cost(route);
                        //CHECK_F(cost_after < cost_before, "Improvement expected!");
                        //CHECK_F(cost_change + cost_before == cost_after, "Change should be equal to predicted");
                    }
                }
            }
            if (!found_improvement && use_dont_look_bits) {
                dont_look_bits[route[i]] = true;
            }
        }
    } while(found_improvement);

    // It may happen that depot is moved to another position
    auto depot_pos = find(begin(route), end(route), 0);
    if (depot_pos != route.begin()) {
        rotate(begin(route), depot_pos, end(route));
    }
    const auto new_travel_cost = instance.calc_travel_cost(route);
    const auto delta = new_travel_cost - old_travel_cost;
    CHECK_F(delta <= 0, "Travel cost should not be grater after 3-opt");
    sol.cost_ += delta;
    LOG_F(INFO, "3-opt improvement: %d", -delta);
    return delta;
}


void three_opt_run_tests() {
    LOG_SCOPE_F(INFO, "three_opt_run_tests");

    vector<int> vec{ 5, 6, 7, 1, 2, 3, 4 };
    WrappedIterator<int> beg(vec, 3);
    LOG_F(INFO, "begin = %d", *beg);
    WrappedIterator<int> end(vec, 3, 4);
    WrappedIterator<int> it(vec, 3, 1);

    rotate(beg, it, end);
    //reverse(beg, it);
    LOG_F(INFO, "After rotate: %s", container_to_string(vec).c_str());
}
