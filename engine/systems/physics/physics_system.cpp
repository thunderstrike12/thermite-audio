#include "physics_system.hpp"
#include "core/logger.hpp"
#include "engine.hpp"

namespace tmt {

void Physics::on_start() { Log::info("Physics on_start"); }

void Physics::on_update(const FrameData& time) {
    Log::info("Physics on_update with delta time: {}", time.delta_time);
}

void Physics::on_fixed_update(const FrameData& time) {
    Log::info(
        "Physics on_fixed_update with delta time: {}",
        Engine::Config::fixed_time_step
    );
}

void Physics::on_end() { Log::info("Physics on_end"); }

}  // namespace tmt
