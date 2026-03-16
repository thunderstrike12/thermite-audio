#include "rendering.hpp"

#include "editor/overlays/lights.hpp"

namespace tmt {

void Rendering::on_draw_lines() const {
    draw_lights_overlay();
}

}  // namespace tmt
