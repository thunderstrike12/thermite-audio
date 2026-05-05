#include "destroy_timed.hpp"
void game::DestroyTimed::update(const tmt::FrameData& time) {
    time_to_destroy -= tmt::engine.frame_data().delta_time;
    if (time_to_destroy < 0.0f) {
        tmt::engine.ecs.destroy_entity(entity);
    }
}
