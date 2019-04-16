#include <ctime>
#include <algorithm>
#include "stopcondition.h"


TimeoutStopCondition::TimeoutStopCondition(double max_seconds) :
    max_seconds_(std::max(0.0, max_seconds)),
    start_time_(0),
    iteration_(0) {
}


void TimeoutStopCondition::start() noexcept {
    start_time_ = clock();
    iteration_ = 0;
}


void TimeoutStopCondition::next_iteration() noexcept {
    ++iteration_;
}


bool TimeoutStopCondition::is_reached() const noexcept {
    const clock_t elapsed = clock() - start_time_;
    return (elapsed / (double)CLOCKS_PER_SEC) > max_seconds_;
}
