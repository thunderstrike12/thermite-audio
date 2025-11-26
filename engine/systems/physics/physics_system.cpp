#include "physics_system.hpp"
#include "core/logger.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/components.hpp"

#include "tools/fmt_defines.hpp"

namespace tmt {

void Physics::on_start() { Log::info("Physics on_start"); }

void Physics::on_update(const FrameData&) {}

void Physics::on_fixed_update(const FrameData&) {}

void Physics::on_end() { Log::info("Physics on_end"); }

}  // namespace tmt
