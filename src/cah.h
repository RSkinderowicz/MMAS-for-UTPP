#ifndef CAH_H
#define CAH_H

#include "tpp.h"
#include "tpp_solution.h"

/**
 * This is an attempt to implement commodity adding heuristic, i.e. CAH
 * as described in
 * Boctor, Fayez F., Gilbert Laporte, and Jacques Renaud. "Heuristics for the
 * traveling purchaser problem." Computers & Operations Research 30.4 (2003):
 * 491-504.
 */
TPP::Solution commodity_adding_heuristic(const TPP::Instance &instance);

#endif
