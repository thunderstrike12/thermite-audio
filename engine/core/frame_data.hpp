#pragma once
#include <cstdint>
#include <chrono>

namespace tmt {

struct FrameData {
    uint64_t frame_number = 0;

    /* In seconds */
    float delta_time = 0;

    /* Time since application started in seconds */
    float elapsed_time = 0.0f;
};

}  // namespace tmt