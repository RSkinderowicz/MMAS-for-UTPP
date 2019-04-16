#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <sys/types.h>
#include <unistd.h>

#include "logging.h"
#include "docopt.h"
#include "utils.h"
#include "tpp.h"
#include "gsh.h"
#include "vec.h"
#include "two_opt.h"
#include "three_opt.h"
#include "drop.h"
#include "rand.h"
#include "cah.h"
#include "aco.h"
#include "tpp_info.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;


static const char USAGE[] =
R"(AntsTPP.

    Usage:
      ants-tpp [--instance=<path>] [--verbosity=<n>] [--trials=<n>]
               [--iterations=<n>] [--timeout=<f>] [--id=<s>]
               [--outdir=<path>] [--alg=<s>] [--seed=<n>]
      ants-tpp (-h | --help)
      ants-tpp --version

    Options:
      --instance=<path>    Path to the instance file.
      --trials=<n>         How many trials to do [default: 1].
      --iterations=<n>     Max number of iterations to perform [default: 1000].
      --timeout=<f>        Timeout in seconds.
      --id=<s>             Identifier of an experiment to which calculations belong [default: default]
      --outdir=<path>      Directory where to store files with results [default: .].
      --alg=<s>            Algorithm to run aco|cah [default: aco].
      --seed=<n>           Initial seed for the pseudo-random num. gen.
                           If 0 current time is used [default: 0]
      -h --help            Show this screen.
      --version            Show version.
      --verbosity=<n>      Verbosity level INFO|WARNING|ERROR [default: WARNING].
)";


enum class Algorithm {
    ACO, CAH
};


void perform_trial(ACO &aco, StopCondition* stop_condition, json &record) {
    clock_t trial_start_time{ 0 };

    vector<int> best_solutions_cost_log;
    vector<int> best_solutions_iteration_log;
    vector<double> best_solutions_time_log;
    vector<double> best_solutions_error_log;

    auto new_best_found_callback = [&](const ACO &aco) {
        const auto time_elapsed_sec = (clock() - trial_start_time)
            / static_cast<double>(CLOCKS_PER_SEC);

        if (aco.global_best_ == nullptr) {
            return ;
        }
        best_solutions_cost_log.push_back(aco.global_best_->cost());
        best_solutions_iteration_log.push_back(aco.current_iteration_);
        best_solutions_time_log.push_back(time_elapsed_sec);

        auto rel_error = aco.global_best_->solution_.get_relative_error() * 100;
        best_solutions_error_log.push_back(rel_error);

        LOG_F(WARNING, "New global best: %d (%.2lf%%, %d), iter: %d",
                aco.global_best_->cost(),
                rel_error,
                aco.instance_.best_known_cost_,
                aco.current_iteration_);
    };

    aco.new_best_found_callback_ = new_best_found_callback;

    trial_start_time = clock();

    aco.run(stop_condition);

    const auto time_elapsed_sec = (clock() - trial_start_time)
        / static_cast<double>(CLOCKS_PER_SEC);

    if (aco.global_best_) {
        LOG_F(WARNING, "Best route: %s",
              container_to_string(aco.global_best_->solution_.route_).c_str());
    }

    record["duration"] = time_elapsed_sec;
    record["total_iterations"] = aco.current_iteration_;
    record["best_solutions_cost_log"] = best_solutions_cost_log;
    record["best_solutions_iteration_log"] = best_solutions_iteration_log;
    record["best_solutions_time_log"] = best_solutions_time_log;
    record["best_solutions_error_log"] = best_solutions_error_log;
}


void perform_trial_cah(const TPP::Instance &instance,
                       StopCondition* stop_condition, json &record) {

    unique_ptr<TPP::Solution> best_solution = nullptr;

    stop_condition->start();

    for ( ; !stop_condition->is_reached(); stop_condition->next_iteration()) {
        const auto sol = commodity_adding_heuristic(instance);

        if (!best_solution
                || best_solution->cost_ > sol.cost_) {
            best_solution = make_unique<TPP::Solution>(sol);

            auto rel_error = best_solution->get_relative_error() * 100;

            LOG_F(WARNING, "New global best: %d (%.2lf%%, %d), iter: %d",
                    best_solution->cost_,
                    rel_error,
                    instance.best_known_cost_,
                    stop_condition->get_iteration());
        }
    }
    if (best_solution) {
        LOG_F(WARNING, "Final solution cost: %d", best_solution->cost_);
    }
}


json record_aco_parameters(const ACO &aco) {
    json record = {
        {"ants", aco.ants_count_},
        {"evaporation_rate", aco.evaporation_rate_},
        {"cand_list_size", aco.cand_list_size_},
        {"local_search_enabled", aco.use_local_search_},
    };
    return record;
}


void init_logging(int argc, char * argv[]) {
    loguru::init(argc, argv);
}


std::string get_result_file_name(const string &label="") {
    string name;

    const auto time_now = std::time(nullptr);
    const auto datetime = localtime(&time_now);

    CHECK_F(datetime != nullptr, "datetime should not be null");

    ostringstream out;
    out << "results_" << label << "_"
        << (datetime->tm_year + 1900) << "-"
        << (datetime->tm_mon + 1) << "-"
        << datetime->tm_mday << "__"
        << datetime->tm_hour << ":"
        << datetime->tm_min << ":"
        << datetime->tm_sec << "_"
        << getpid()
        << ".js";

    return out.str();
}


int main(int argc, char *argv[])
{
    std::map<std::string, docopt::value> args
        = docopt::docopt(USAGE,
                         { argv + 1, argv + argc },
                         true,  // show help if requested
                         "AntsTPP 0.1 by Rafal Skinderowicz");  // version string

    if (args.count("--verbosity")) {
        vector<char*> new_args{ argv, argv + argc };
        const auto verbosity_flag = "-v";
        new_args.push_back(const_cast<char*>(verbosity_flag));
        const auto level = args["--verbosity"].asString();
        new_args.push_back(const_cast<char*>(level.c_str()));
        new_args.push_back(nullptr);

        init_logging(argc + 2, new_args.data());
    } else {
        init_logging(argc, argv);
    }

    if (args.count("--seed")) {
        const auto value = args["--seed"].asLong();
        if (value != 0) {
            set_initial_seed(static_cast<uint32_t>(value));
        }
    }

    TPP::run_tests();
    Vec::run_tests();
    test_two_opt();
    three_opt_run_tests();

    auto outdir = args["--outdir"].asString();
    make_path(outdir);

    if (args.count("--instance")) {
        const auto path = args["--instance"].asString();

        LOG_F(INFO, "Instance path given: %s", path.c_str());

        auto instance = TPP::load_from_file(path);

        if (instance.is_capacitated_ == true) {
            LOG_F(ERROR, "Uncapacitated TPP instance required");
            abort();
        }

        const auto best_known = TPP::get_best_known_solution(path);
        instance.best_known_cost_ = best_known.cost_;

        auto alg = Algorithm::ACO;

        if (args.count("--alg")) {
            const auto name = args["--alg"].asString();
            if (name == "aco") {
                alg = Algorithm::ACO;
            } else if (name == "cah") {
                alg = Algorithm::CAH;
            } else {
                CHECK_F(false, "Unknown algorithm: %s", name.c_str());
            }
        }

        auto trials = 1;
        if (args.count("--trials")) {
            trials = args["--trials"].asLong();
        }

        json record;

        record["experiment_id"] = args["--id"].asString();

        unique_ptr<StopCondition> stop_condition{ nullptr };
        if (args["--timeout"]) {
            const auto timeout_str = args["--timeout"].asString();
            const auto timeout_sec = std::atof(timeout_str.c_str());

            stop_condition = make_unique<TimeoutStopCondition>(timeout_sec);

            record["timeout"] = timeout_sec;
        } else {
            const auto max_iterations = args["--iterations"].asLong();
            if (max_iterations > 0) {
                stop_condition = make_unique<FixedIterationsStopCondition>(max_iterations);
            }
            record["max_iterations"] = max_iterations;
        }
        CHECK_F(stop_condition != nullptr, "Stop condition should be initialized");

        record["trials_count"] = trials;
        record["instance_path"] = path;
        record["instance_name"] = instance.name_;
        record["instance_dimension"] = instance.dimension_;
        record["instance_product_count"] = instance.product_count_;
        record["best_known_cost"] = instance.best_known_cost_;
        record["rng_seed"] = get_initial_seed();

        json trials_record = json::array();

        int best_found_cost = numeric_limits<int>::max();
        vector<uint32_t> best_found_solution;
        double best_found_error = -1;
        vector<int> trials_best_cost;
        vector<double> trials_best_error;

        for (auto trial = 0; trial < trials; ++trial) {
            json trial_record;

            if (alg == Algorithm::ACO) {
                ACO aco(instance);
                perform_trial(aco, stop_condition.get(), trial_record);
                trials_record.push_back(trial_record);

                if (!aco.global_best_) {
                    break ;
                }
                const auto &best_ant = *aco.global_best_;
                if (best_ant.cost() < best_found_cost) {
                    best_found_cost = best_ant.cost();
                    best_found_solution = best_ant.solution_.route_;
                    best_found_error = best_ant.solution_.get_relative_error();
                }
                trials_best_cost.push_back(best_ant.cost());
                trials_best_error.push_back(best_ant.solution_.get_relative_error());

                record["aco_parameters"] = record_aco_parameters(aco);
            } else if (alg == Algorithm::CAH) {
                perform_trial_cah(instance, stop_condition.get(), trial_record);
            }
        }
        record["trials"] = trials_record;

        record["best_found_cost"] = best_found_cost;
        record["best_found_error"] = best_found_error;
        record["best_found_solution"] = best_found_solution;
        record["mean_best_solution_cost"] = sample_mean(trials_best_cost);
        record["mean_best_solution_error"] = sample_mean(trials_best_error);

        auto result_file_path = outdir + "/"
                              + get_result_file_name(instance.name_);
        LOG_F(WARNING, "Saving results to a file: %s", result_file_path.c_str());

        ofstream outf(result_file_path);
        if (outf.is_open()) {
            outf << record.dump(2);
        } else {
            LOG_F(ERROR, "Cannot create a file with results: %s", result_file_path.c_str());
        }
    }
    return EXIT_SUCCESS;
}