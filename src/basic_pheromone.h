#pragma once

#include <vector>
#include <cstdint>


/**
 * This is an implementation of a standard (basic) version of pheromone memory
 * in which a matrix is used to store current pheromone level for each of
 * available solution components.
 */
struct BasicPheromone {
    std::vector<std::vector<double>> trails_;
    bool is_symmetric_{ false };
    double min_value_{ 0 };
    double max_value_{ 1 };


    BasicPheromone(uint32_t size, bool is_symmetric,
                   double min_value, double max_value);

    double get_trail(uint32_t from, uint32_t to) const noexcept;

    void increase(uint32_t from, uint32_t to, double delta);

    void evaporate(double evaporation_ratio);

    void set_all_trails(double value);

    void set_trail_limits(double min_value, double max_value);
};
