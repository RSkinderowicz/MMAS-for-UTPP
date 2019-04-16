#ifndef ACO_H
#define ACO_H

/*
 * ACO-based algorithm for the TPP
 */

#include <memory>
#include <array>
#include <functional>

#include "tpp_solution.h"
#include "ant.h"
#include "stopcondition.h"
#include "basic_pheromone.h"


struct ACO {
    using callback_t = void (const ACO & aco);

    const TPP::Instance &instance_;
    std::unique_ptr<BasicPheromone> pheromone_;
    std::vector<std::shared_ptr<Ant>> ants_;
    std::shared_ptr<Ant> global_best_{ nullptr };

    size_t ants_count_ = 20;
    double evaporation_rate_ = 0.99;
    size_t cand_list_size_ = 25;
    bool use_local_search_ = true;

    double initial_pheromone_ = 0;
    double min_pheromone_ = 0;
    double max_pheromone_ = 0;
    int greedy_solution_value_ = 0;
    int global_best_cost_no_ls_ = 0;
    std::vector<int> global_best_values_no_ls_;
    int current_iteration_ = 0;
    // restart_best_ -> the best ant since the last restart
    std::shared_ptr<Ant> restart_best_{ nullptr };
    int restart_best_found_iteration_ = 0;
    int pheromone_reset_iteration_ = 0;
    int u_gb_ = 25;

    std::shared_ptr<Ant> iteration_best_{ nullptr };

    // [m][p] = value of a heuristic for product p at market m
    std::vector<std::vector<double>> heuristic_;
    std::vector<std::vector<uint32_t>> ant_phmem_samples_;

    // Callbacks
    std::function<callback_t> new_best_found_callback_{ nullptr };


    ACO(TPP::Instance &instance);

    /**
     * Runs the algorithm until stop_condition is reached.
     */
    void run(StopCondition *stop_condition);

private:

    void run_init();

    void calc_initial_pheromone();

    void build_ant_solutions();

    void move_ant(Ant &ant);

    double calc_attractiveness(Ant &ant, size_t market);

    void init_heuristic_info();

    void update_u_gb() noexcept;

    /**
     * This applies local search (if enabled) to selected ants' solutions.
     */
    void apply_local_search();
};


#endif
