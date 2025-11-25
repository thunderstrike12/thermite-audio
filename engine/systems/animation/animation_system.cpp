#include "animation_system.hpp"

#include "core/logger.hpp"

namespace tmt {

void Animation::on_start() { Log::info("Animation on_start"); }

void Animation::on_update(const FrameData&) {
    Log::info("Animation on_update");
}

void Animation::on_end() { Log::info("Animation on_end"); }

}  // namespace tmt
