#include <fstream>
#include <iostream>

#include "tpp_info.h"
#include "json.hpp"
#include "logging.h"


using namespace std;
using namespace TPP;
using json = nlohmann::json;


/**
 * Returns the best known solution or SolutionInfo{ 0, 0 } if it is not
 * available.
 */
TPP::SolutionInfo TPP::get_best_known_solution(std::string instance_path) {
    LOG_SCOPE_F(INFO, "TPP::get_best_known_solution(%s)",
                instance_path.c_str());

    SolutionInfo info{ 0, 0 };

    static json db;

    if (db.empty()) {
        ifstream fin("best-known.js");
        if (fin) {
            fin >> db;
        }
    }
    // A naive extraction of filename
    auto it = instance_path.find_last_of("/");
    const auto filename = (it == string::npos) ? instance_path
                                               : instance_path.substr(it + 1);
    LOG_F(INFO, "Instance filename: %s", filename.c_str());

    bool found = false;
    for (auto& element : db) {
        if (element["name"] == filename) {
            info.cost_ = element["best_cost"];
            info.markets_count_ = element["best_markets"];
            found = true;
            break ;
        }
    }
    if (!found) {
        LOG_F(WARNING,
              "No info about best known solution for the instance at path: %s",
              instance_path.c_str());
    }
    return info;
}
