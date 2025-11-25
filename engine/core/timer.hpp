#pragma once
#include <chrono>

namespace tmt {

class Timer {
   private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    TimePoint start_time;
    TimePoint last_time;

   public:
    Timer() : start_time(Clock::now()), last_time(start_time) {}

    float tick() {
        auto current = Clock::now();
        float delta = std::chrono::duration<float>(current - last_time).count();
        last_time = current;
        return delta;
    }

    float elapsed() const { return std::chrono::duration<float>(Clock::now() - start_time).count(); }

    void reset() { start_time = last_time = Clock::now(); }
};
};  // namespace tmt