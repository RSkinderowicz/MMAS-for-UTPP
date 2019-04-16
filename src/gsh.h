#ifndef GSH_H
#define GSH_H


/*
 * An impl. of the Generalized Savings Heuristic as given by:
 *
 * Golden, B. , Levy, L. , & Dahl, R. (1981). Two generalizations of the traveling salesman
 * problem. Omega, 9 (4), 439–441
 */

#include "tpp.h"
#include "tpp_solution.h"


TPP::Solution calc_gsh_solution(const TPP::Instance& instance);


#endif
