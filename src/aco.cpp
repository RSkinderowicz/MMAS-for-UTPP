#include <algorithm>
#include <cmath>
#include <numeric>

#include "aco.h"
#include "cah.h"
#include "logging.h"
#include "rand.h"
#include "drop.h"
#include "three_opt.h"
#include "two_opt.h"


using namespace std;


void local_search(const TPP::Instance &instance,
                 TPP::Solution &sol,
                 int global_best_cost) {
    auto improvement_found = false;
    auto pass = 0;
    constexpr auto MaxPasses = 2;
    bool global_best_improved = false;

    three_opt_nn(instance, sol, /*don't look bits=*/true, /*nn_count=*/25);

    do {
        improvement_found = false;
        const auto start_cost = sol.cost_;

        drop_heuristic(instance, sol);
        insertion_heuristic(instance, sol);
        k_exchange_heuristic(instance, sol, 3);
        double_exchange_heuristic(instance, sol);
        exchange_heuristic(instance, sol);

        if (sol.cost_ != start_cost) {
            three_opt_nn(instance, sol, /*don't look bits=*/true, /*nn_count=*/25);
        }
        improvement_found = (sol.cost_ < start_cost);
        ++pass;
        if (improvement_found && sol.cost_ < (global_best_cost * (1. + 0.08/(pass * pass)))) {
            global_best_improved = true;
        }
    } while(improvement_found && (pass < MaxPasses || global_best_improved));
    CHECK_F(is_solution_valid(instance, sol.route_), "Sol should be valid");
}


/**
* Computes the average node lambda-branching factor.
*
* This is based on the ACOTSP software by T. Stutzle as available at:
* http://www.aco-metaheuristic.org/aco-code/public-software.html
*/
template<typename pheromone_t>
double node_branching(double lambda,
                      size_t cand_list_size,
                      const pheromone_t &pheromone,
                      const TPP::Instance &problem) {
    const auto n = problem.dimension_;
    vector<int> num_branches(n);
    const auto nn_ants = cand_list_size;

    assert(nn_ants > 0);

    for (auto m = 0u ; m < n ; m++) {
        auto &nn_list = problem.nn_lists_.at(m);
        /* determine max, min to calculate the cutoff value */
        auto min = pheromone.get_trail(m, nn_list.at(0));
        auto max = min;
        for (auto i = 1u ; i < nn_ants ; i++) {
            const auto nn = nn_list.at(i);
            const auto ph = pheromone.get_trail(m, nn);
            if (ph > max) {
                max = ph;
            }
            if (ph < min) {
                min = ph;
            }
        }
        auto cutoff = min + lambda * (max - min);

        for (auto i = 0u; i < nn_ants ; i++) {
            const auto nn = nn_list.at(i);
            if (pheromone.get_trail(m, nn) > cutoff) {
                ++num_branches.at(m);
            }
        }
    }
    auto avg = 0.0;
    for (auto m = 0u ; m < n ; m++) {
        avg += num_branches[m];
    }
    /* Norm branching factor to minimal value 1 */
    double branching_factor = (avg / (double) (n * 2));
    return branching_factor;
}


ACO::ACO(TPP::Instance &instance)
    : instance_(instance)
{}


/**
    * Runs the algorithm until stop_condition is reached.
    */
void ACO::run(StopCondition *stop_condition) {
    LOG_SCOPE_F(INFO, "run");

    stop_condition->start();

    run_init();

    // For sorting ants according to the cost of a solution
    auto cmp = [](auto l, auto r) { return l->cost() < r->cost(); };

    for ( ; !stop_condition->is_reached(); stop_condition->next_iteration()) {
        build_ant_solutions();

        iteration_best_ = *min_element(begin(ants_), end(ants_), cmp);

        const auto cost = iteration_best_->cost();
        if (!global_best_cost_no_ls_ || global_best_cost_no_ls_ > cost) {
            global_best_cost_no_ls_ = cost;
            global_best_values_no_ls_.push_back(cost);
        }

        apply_local_search();

        iteration_best_ = *min_element(begin(ants_), end(ants_), cmp);

        if (!global_best_ || global_best_->cost() > iteration_best_->cost()) {
            global_best_ = make_shared<Ant>(*iteration_best_);

            //pheromone_->add_solution(global_best_->solution_.route_,
                                    //global_best_->cost());

            if (new_best_found_callback_) {
                new_best_found_callback_(*this);
            }
        } else {
            //pheromone_->add_solution(global_best_->solution_.route_,
                                    //global_best_->cost());
            //const auto threshold = global_best_->cost() * 1.01;

            //if (iteration_best_->cost() < threshold) {
                //pheromone_->add_solution(iteration_best_->solution_.route_, iteration_best_->cost());
            //}
        }

        if (!restart_best_ || restart_best_->cost() > iteration_best_->cost()) {
            restart_best_ = make_shared<Ant>(*iteration_best_);
            restart_best_found_iteration_ = current_iteration_;
        }

        // Update pheromone levels' limits
        const auto best_cost = global_best_->cost();
        max_pheromone_ = 1. / (best_cost * evaporation_rate_);
        min_pheromone_ = max_pheromone_ / (2 * instance_.dimension_);

        // Evaporate & update pheromone
        pheromone_->set_trail_limits(min_pheromone_, max_pheromone_);

        pheromone_->evaporate(evaporation_rate_);

        Ant* update_ant = nullptr;

        if (current_iteration_ % u_gb_ != 0) {
            update_ant = iteration_best_.get();
        } else {
            if (u_gb_ == 1
                && (current_iteration_ - restart_best_found_iteration_) > 50) {
                update_ant = global_best_.get();
            } else {
                update_ant = restart_best_.get();
            }
        }

        double deposit = 1. / update_ant->cost();
        auto prev = update_ant->get_route().back();
        for (auto market : update_ant->get_route()) {
            pheromone_->increase(prev, market, deposit);
            prev = market;
        }

        if ((current_iteration_ + 1) % 100 == 0) {
            const auto lambda = 0.05;
            const auto branching_factor_threshold = 1.00001;
            const auto branching_factor = node_branching(lambda, cand_list_size_,
                                                         *pheromone_, instance_);

            LOG_F(WARNING, "Branching factor: %lf", branching_factor);

            if ((current_iteration_ - restart_best_found_iteration_ > 250)
                && branching_factor < branching_factor_threshold) {

                LOG_F(WARNING, "Resetting pheromone at iteration: %d",
                      current_iteration_);

                pheromone_->set_all_trails(max_pheromone_);
                restart_best_ = nullptr;
                pheromone_reset_iteration_ = current_iteration_;

                global_best_cost_no_ls_ = numeric_limits<int>::max();
                global_best_values_no_ls_.clear();
            }
        }
        ++current_iteration_;
        update_u_gb();
    }

    LOG_F(INFO, "Final best value: %d", global_best_->cost());
    LOG_F(INFO, "Best ant affinity: %lf", global_best_->affinity_);
    LOG_F(INFO, "Best ant laziness_: %lf", global_best_->laziness_);
    LOG_F(INFO, "Best ant avidity_: %lf", global_best_->avidity_);
}


void ACO::run_init() {
    global_best_ = nullptr;
    global_best_cost_no_ls_ = 0;
    global_best_values_no_ls_.clear();

    restart_best_ = nullptr;
    restart_best_found_iteration_ = 0;

    if (initial_pheromone_ == 0) {
        calc_initial_pheromone();
    }

    pheromone_ = make_unique<BasicPheromone>(instance_.dimension_,
                                             instance_.is_symmetric_,
                                             min_pheromone_,
                                             max_pheromone_);
    init_heuristic_info();

    current_iteration_ = 0;
}


void ACO::calc_initial_pheromone() {
    LOG_SCOPE_F(INFO, "calc_initial_pheromone");
    if (greedy_solution_value_ == 0) {
        auto sol = commodity_adding_heuristic(instance_);
        greedy_solution_value_ = sol.cost_;
        /*restart_best_ = make_shared<Ant>(instance_);
        for (auto node : sol.route_) {
            if (node != 0) {
                restart_best_->move_to(node);
            }
        }
        LOG_F(WARNING, "Greedy val: %d", restart_best_->cost());
        global_best_ = restart_best_;*/
    }
    max_pheromone_ = 1. / (greedy_solution_value_ * evaporation_rate_);
    min_pheromone_ = max_pheromone_ / (2 * instance_.dimension_);
    initial_pheromone_ = max_pheromone_;

    LOG_F(INFO, "max_pheromone_: %lf", max_pheromone_);
    LOG_F(INFO, "min_pheromone_: %lf", min_pheromone_);
}


void ACO::build_ant_solutions() {
    LOG_SCOPE_F(INFO, "build_ant_solutions");
    ants_.clear();

    ant_phmem_samples_.resize(ants_count_);
    for (auto i = 0u; i < ants_count_; ++i) {
        ants_.push_back(make_shared<Ant>(instance_));
        ants_.back()->id_ = i;
        // ant_phmem_samples_[i] = random_sample(pheromone_->routes_count_, get_random_uint(2, 8));
    }

    for (auto i = 1u; i < instance_.dimension_; ++i) {
        for (auto &ant : ants_) {
            move_ant(*ant);
        }
    }
    for (auto &ant : ants_) {
        CHECK_F(ant->solution_.is_valid(), "Ant solution should be valid");
        CHECK_F(ant->solution_.cost_ == calc_solution_cost(instance_, ant->solution_.route_),
                "Sol. cost should be valid (%d != %d)",
                ant->solution_.cost_,
                calc_solution_cost(instance_, ant->solution_.route_));
        // Remove unnecessary markets from the solution:
        drop_heuristic(instance_, ant->solution_);
    }
}


void ACO::move_ant(Ant &ant) {
    if (ant.solution_.is_valid()) {
        const auto oversize = ant.oversize_;
        const size_t delta = std::round(ant.length_when_valid_ * oversize);
        const size_t trials = instance_.dimension_ - ant.length_when_valid_;
        const double p = static_cast<double>(delta) / trials;
        if (delta == 0 || get_random_value() > p) {
            return ;
        }
    }
    const auto &cand = ant.get_candidate_markets(cand_list_size_);

    CHECK_F( !cand.empty(), "At least one market should be unvisited");

    static vector<double> cand_values(instance_.dimension_);
    cand_values.clear();
    auto total = 0.0;
    for (auto m : cand) {
        const auto v = calc_attractiveness(ant, m);
        cand_values.push_back(v);
        total += v;
    }
    const auto threshold = get_random_value() * total;
    auto partial_sum = 0.0;
    auto chosen = cand.back();
    CHECK_F(chosen != 0, "back() should not be a depot!");
    for (auto i = 0ul, n = cand_values.size(); i < n; ++i) {
        partial_sum += cand_values[i];
        if (partial_sum >= threshold) {
            chosen = cand.at(i);
            CHECK_F(chosen != 0, "No depot");
            break ;
        }
    }
    CHECK_F(chosen != 0, "Cannot move to depot!");
    ant.move_to(chosen);
}


double ACO::calc_attractiveness(Ant &ant, size_t to_market) {
    const auto from_market = ant.get_position();
    const auto &phmem_indices = ant_phmem_samples_.at(ant.id_);
    // const auto trail = pheromone_->get_trail_sample(from_market, to_market, phmem_indices);
    const auto trail = pheromone_->get_trail(from_market, to_market);

    double product = std::pow(trail, static_cast<int>(ant.affinity_));

    const auto travel_cost = instance_.get_travel_cost(from_market, to_market);
    product *= std::pow(1./travel_cost, static_cast<int>(ant.laziness_));

    auto h = heuristic_.at(to_market).at(instance_.product_count_);
    product *= std::pow(max(1.e-10, h), static_cast<int>(ant.avidity_));

    // ucc = updated commodity cost
    //double ucc = ant.solution_.calc_market_add_cost(to_market).cost_change_;
    //product *= std::pow(1./max(1., ucc), static_cast<int>(ant.avidity_));
    return product;
}


TPP::Solution create_random_solution(const TPP::Instance &instance) {
    TPP::Solution sol(instance);
    auto unselected = sol.get_unselected_markets();

    shuffle_vector(unselected);

    for (auto market : unselected) {
        sol.push_back_market(market);
        if (sol.is_valid()) {
            break ;
        }
    }
    drop_heuristic(instance, sol);
    return sol;
}


void ACO::init_heuristic_info() {
    LOG_SCOPE_F(INFO, "init_heuristic_info");

    heuristic_.clear();
    heuristic_.resize(instance_.dimension_);
    for (auto &vec : heuristic_) {
        vec.resize(instance_.product_count_ + 1, 0);
    }

    // [i][j] = how many units of a product j were bought at market i
    vector<vector<double>> bought_at_markets(instance_.dimension_);
    for (auto &vec : bought_at_markets) {
        vec.resize(instance_.product_count_, 0);

    }

    const auto trials = 200;
    for (auto i = 0; i < trials; ++i) {
        auto sol = create_random_solution(instance_);
        double purchases_cost = accumulate(begin(sol.purchase_costs_),
                                           end(sol.purchase_costs_), 0);
        for (const auto &offers : sol.product_offers_) {
            CHECK_F(!offers.empty(), "At least one offer should be used");

            const auto &first = offers.front();
            const auto product_id = first.product_id_;
            const auto needed = instance_.demands_.at(product_id);
            int total_bought = 0;
            for (const auto &offer : offers) {
                const auto bought = min(offer.quantity_, needed - total_bought);
                bought_at_markets.at(offer.market_id_).at(product_id) +=
                    (bought * offer.price_) / purchases_cost;

                total_bought -= bought;
                if (bought == 0) {
                    break ;  // No need to check further as offers are sorted
                             // according to price
                }
            }
        }
    }
    for (auto m = 0u; m < instance_.dimension_; ++m) {
        double sum = 0;
        for (auto p = 0u; p < instance_.product_count_; ++p) {
            const double bought = bought_at_markets.at(m).at(p);
            const auto ratio = bought / trials;
            heuristic_.at(m).at(p) = ratio;
            sum += ratio;
        }
        heuristic_.at(m).at(instance_.product_count_) = sum;
    }
}


/**
 * Implements the schedule for u_gb as defined in the
 * Future Generation Computer Systems article or in Stuetzle's PhD thesis.
 * This schedule is only applied if local search is used.
 */
void ACO::update_u_gb() noexcept {
    if (use_local_search_) {
        const auto delta = current_iteration_ - restart_best_found_iteration_;
        if (delta < 25)
            u_gb_ = 25;
        else if (delta < 75)
            u_gb_ = 5;
        else if (delta < 125)
            u_gb_ = 3;
        else if (delta < 250)
            u_gb_ = 2;
        else
            u_gb_ = 1;
    } else {
        u_gb_ = 25;
    }
}


/**
 * This applies local search (if enabled) to selected ants' solutions.
 */
void ACO::apply_local_search() {
    if (current_iteration_ == 200) {
        pheromone_->set_all_trails(max_pheromone_);
    }
    // Calc. local search application threshold
    if (use_local_search_ && current_iteration_ >= 200) {
        const auto crude_threshold = global_best_cost_no_ls_ * 1.02;
        const auto track_size = global_best_values_no_ls_.size();
        const auto in_track_index = track_size - min(track_size, 5ul);
        const auto track_threshold = global_best_values_no_ls_[in_track_index];
        //const auto threshold = max(crude_threshold,
                                   //static_cast<double>(track_threshold));
        for (auto &ant : ants_) {
            if (ant->cost() <= track_threshold) {
                local_search(instance_, ant->solution_, global_best_->cost());
            }
        }
    }
}
