#include "motion_math_system.hpp"
#include "glm/gtx/quaternion.hpp"

void tmt::MotionMathSystem::on_start() {}

void tmt::MotionMathSystem::on_fixed_update(const tmt::FrameData& time) {
    for (auto& [type_index, motion_bucket] : motion_buckets) {
        motion_bucket.update_method(motion_bucket.motion_collection, time.delta_time);
    }
}

void tmt::MotionMathSystem::on_end() { motion_buckets.clear(); }