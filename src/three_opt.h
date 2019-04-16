#ifndef THREE_OPT_H
#define THREE_OPT_H


#include "tpp_solution.h"


/*
 * Impl. of the 3-opt heuristic. Tries to change the order of markets in
 * solution to shorten the travel distance.
 *
 * The solution's route is modified only if a better order was found.
 *
 * Returns improvement over the previous route length (travel distance).
 */
int three_opt(const TPP::Instance &instance, TPP::Solution &sol,
              bool use_dont_look_bits=true);


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
                 bool use_dont_look_bits=true, size_t nn_count=30);


void three_opt_run_tests();


#endif

