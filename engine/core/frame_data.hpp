#pragma once
#include <cstdint>
#include <chrono>

namespace tmt {

struct FrameData {
    uint64_t frame_number = 0;

    /* In seconds, capped to MAX_DELTA_TIME */
    float delta_time = 0.0f;

    /* In seconds, not capped */
    float uncapped_delta_time = 0.0f;

    /* Time since application started in seconds */
    float elapsed_time = 0.0f;
};

}  // namespace tmt