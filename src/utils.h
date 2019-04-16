#ifndef UTILS_H
#define UTILS_H


#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <numeric>
#include <cmath>


/**
 * This restores heap property for elements [pos, pos + 1, ..., last)
 * The elements [begin, pos) should already have the binary heap property.
 */
template<typename RandomIt, typename Compare>
void restore_heap(RandomIt begin, RandomIt pos, RandomIt last, Compare cmp) {
    while (pos != last) {
        ++pos;
        std::push_heap(begin, pos, cmp);
    }
    if (begin != last) {
        std::push_heap(begin, last, cmp);
    }
}


/**
 * Inserts element into sorted sequence.
 */
template<typename T, typename Pred>
typename std::vector<T>::iterator
insert_sorted(std::vector<T> & vec, T const& item, Pred pred) {
    return vec.insert(
           std::upper_bound(vec.begin(), vec.end(), item, pred),
           item
        );
}


/**
 * Trims the string from the left.
 */
void ltrim(std::string &s);


/**
 * Trims the string from the right.
 */
void rtrim(std::string &s);


/**
 * Trims from both ends (in place)
 */
void trim(std::string &s);


bool starts_with(const std::string &text, const std::string &prefix);


template<typename T>
double sample_mean(const std::vector<T> &vec) {
    assert(!vec.empty());
    const auto sum = std::accumulate(begin(vec), end(vec), 0.0);
    const auto mean = sum / vec.size();
    return mean;
}


template<typename T>
double sample_stdev(const std::vector<T> &vec) {
    if (vec.size() <= 1) {
        return 0.0;
    }
    std::vector<double> diff(vec.size());
    const auto mean = calc_mean(vec);
    transform(vec.begin(), vec.end(), diff.begin(),
            [mean](double x) { return x - mean; });
    double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / (vec.size() - 1));
    return stdev;
}


/**
 * Creates a list of directories as specified in the path.
 * Linux only
 */
bool make_path(const std::string& path);


#endif
