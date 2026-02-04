#include "rendering.hpp"

#include "editor/overlays/lights.hpp"

namespace tmt {

void Rendering::on_editor_start() {}

void Rendering::on_editor_update(const tmt::FrameData&) {}

void Rendering::on_editor_end() {}

void Rendering::display() {}

void Rendering::on_draw_lines() const { draw_lights_overlay(); }

}  // namespace tmt
