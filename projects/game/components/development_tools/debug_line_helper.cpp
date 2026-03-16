#include "debug_line_helper.hpp"

#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"
void game::DebugLineConfig::set_values() const {
    auto& poly = tmt::engine.polyline;
    poly.use_line_width(line_width);
    poly.use_color(color);
}
