#pragma once
#include <chrono>
#include <thread>

namespace tmt {

class FpsLimiter {
   public:
    FpsLimiter() = default;
    FpsLimiter(float target_fps) : target_frame_time(1.0 / target_fps), target_fps(target_fps), enabled(true) {}

    void begin() { frame_start = Clock::now(); }

    void end() {
        if (enabled == false) return;
        const Duration remaining = target_frame_time - (Clock::now() - frame_start);
        if (remaining.count() <= 0.0) return;

        const Duration sleep_dur = remaining - std::chrono::milliseconds(1);
        if (sleep_dur.count() > 0.0) std::this_thread::sleep_for(sleep_dur);

        while (Clock::now() - frame_start < target_frame_time) std::this_thread::yield();
    }

    void set_target_fps(float fps) {
        target_frame_time = Duration(1.0 / fps);
        enabled = true;
        target_fps = fps;
    }
    float get_target_fps() const { return target_fps; }

    void disable() { enabled = false; }
    bool is_enabled() const { return enabled; }

   private:
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::duration<double>;

    bool enabled = false;
    Duration target_frame_time;
    Clock::time_point frame_start;
    float target_fps = 0;
};

}  // namespace tmt