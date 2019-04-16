#ifndef LOGGING_H
#define LOGGING_H

#include "loguru.hpp"

#include <sstream>


template<typename T>
std::string
container_to_string(const T& container, std::string delimeter = " ") {
    std::stringstream ss;
    bool after_first = false;
    for (const auto &el : container) {
        if (after_first) {
            ss << delimeter;
        }
        ss << el;
        after_first = true;
    }
    std::string s = ss.str();
    return s;
}

#endif
