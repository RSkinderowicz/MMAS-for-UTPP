#include "two_opt.h"
#include "logging.h"

#include <algorithm>

using namespace std;
using namespace TPP;

/*
 * A basic version of the 2-opt heuristic.
 *
 * Returns improvement over the previous route length
 */
int two_opt(const TPP::Instance &instance,
            std::vector<uint32_t> &route) {

    LOG_SCOPE_F(INFO, "two_opt");
    CHECK_F(instance.is_symmetric_, "Expected symmetric instance");

    int total_improvement = 0;

    const auto len = route.size();

    auto improvement_found = false;
    do {
        improvement_found = false;
        int best_change_value = 0;
        auto best_change_beg = len;
        auto best_change_end = len;

        for (auto i = 1u; i < len - 1; i++) {
            const auto a = route[i];
            const auto a_prev = route[i-1];

            for (auto j = i + 1u; j < len; ++j) {
                const auto b = route[j];
                const auto b_next = route[(j + 1) % len];

                auto diff = instance.get_travel_cost(a_prev, a)
                          + instance.get_travel_cost(b, b_next)
                          - instance.get_travel_cost(a_prev, b)
                          - instance.get_travel_cost(a, b_next);
                if (diff > best_change_value) {
                    best_change_value = diff;
                    best_change_beg = i;
                    best_change_end = j;
                }
            }
        }
        if (best_change_value > 0) {
            LOG_F(INFO, "best_change_value: %d", best_change_value);
            auto beg = route.begin() + static_cast<int>(best_change_beg);
            auto end = route.begin() + static_cast<int>(best_change_end);
            ++end;
            reverse(beg, end);
            total_improvement += best_change_value;
            improvement_found = true;
        }
    } while(improvement_found);
    return total_improvement;
}


/*
 * A basic version of the 2-opt heuristic.
 *
 * Returns improvement over the previous route length
 */
int two_opt(const TPP::Instance &instance,
            TPP::Solution &sol) {
    const auto start_cost = sol.cost_;
    const auto improvement = two_opt(instance, sol.route_);
    sol.cost_ -= improvement;
    return improvement;
}


void test_two_opt() {
    LOG_SCOPE_F(INFO, "test_two_opt");
    vector<int> weights{ 0, 2, 1, 1,
                         2, 0, 1, 1,
                         1, 1, 0, 1,
                         1, 1, 1, 0 };


    Instance instance;
    instance.dimension_ = 4;
    instance.edge_weights_1d_ = weights;
    instance.is_symmetric_ = true;

    vector<uint32_t> sol1{ 0, 1, 2, 3 };
    CHECK_F(two_opt(instance, sol1) == 1, "Improvement exp.");
    LOG_F(INFO, "route after 2-opt: %s", container_to_string(sol1).c_str());
}
