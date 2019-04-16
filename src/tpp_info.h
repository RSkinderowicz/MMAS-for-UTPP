#ifndef TPP_INFO_H
#define TPP_INFO_H

#include <string>


namespace TPP {

struct SolutionInfo {
    int cost_{ 0 };
    int markets_count_{ 0 };
};


/**
 * Returns the best known solution or SolutionInfo{ 0, 0 } if it is not
 * available.
 */
SolutionInfo get_best_known_solution(std::string tpp_instance_path);

}


#endif
