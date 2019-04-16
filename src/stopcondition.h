#ifndef STOPCONDITION
#define STOPCONDITION

#include <cstdint>
#include <ctime>


struct StopCondition {
    /**
     * This should be called before the first use of the other methods.
     */
    virtual void start() noexcept = 0;

    /**
     * This should be called after each iteration.
     */
    virtual void next_iteration() noexcept = 0;

    /**
     * This method returns true when the stopping criterion has been reached,
     * i.e. the maximum number of iteration has been performed or the time limit
     * has been exceeded
     */
    virtual bool is_reached() const noexcept = 0;

    /**
     * Returns current iteration number
     */
    virtual uint32_t get_iteration() const = 0;
};



struct FixedIterationsStopCondition : StopCondition {

    FixedIterationsStopCondition(const uint32_t max_iterations) :
        iteration_(0),
        max_iterations_(max_iterations) {
    }

    void start() noexcept override {
        iteration_ = 0;
    }

    void next_iteration() noexcept override {
        if (iteration_ < max_iterations_) {
            ++iteration_;
        }
    }

    bool is_reached() const noexcept override {
        return iteration_ == max_iterations_;
    }

    uint32_t get_iteration() const noexcept override {
        return iteration_;
    }


    uint32_t iteration_;
    uint32_t max_iterations_;
};



struct TimeoutStopCondition : StopCondition {

    TimeoutStopCondition(double max_seconds);

    void start() noexcept override;

    void next_iteration() noexcept override;

    bool is_reached() const noexcept override;

    uint32_t get_iteration() const noexcept override {
        return iteration_;
    }


    double max_seconds_;
    clock_t start_time_;
    uint32_t iteration_;
};

#endif
