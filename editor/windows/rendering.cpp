#include "rendering.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/pipelines/vfx_pipeline.hpp"
#include "engine/core/renderer/pipelines/ui_pipeline.hpp"
#include "editor/overlays/lights.hpp"

namespace tmt {

void Rendering::on_draw_lines() const {
    draw_lights_overlay();
}

void Rendering::on_inspect() {
    ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "Geometry Statistics");
    ImGui::Text("Objects: %d", engine.renderer.scene_view.object_count);

    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "Lighting Statistics");
    ImGui::Text("Lights: %d", engine.renderer.scene_view.light_count);
    ImGui::Text("Sun Light:");
    ImGui::SameLine();
    if (engine.renderer.scene_view.sun_light_active) {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Active");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Inactive");
    }

    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "VFX Statistics");
    ImGui::Text("Emitters: %d", engine.renderer.vfx_pipeline.emitter_count);
    ImGui::Text("Effects: %d", engine.renderer.vfx_pipeline.effects_count);

    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "UI Statistics");
    ImGui::Text("2D Images: %d", engine.renderer.ui_pipeline.image_count_2d);
    ImGui::Text("3D Images: %d", engine.renderer.ui_pipeline.image_count_3d);
}

}  // namespace tmt
