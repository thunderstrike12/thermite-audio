#include "material_editor.hpp"

#include "editor.hpp"
#define IM_CUSTOM_COLORSPACE_MATRIX { 0.597314f, 0.073732f, 0.020689f, 0.332820f, 0.917381f, 0.118816f, 0.038138f, 0.017072f, 0.961536f }
#include "editor/shared/imgui_color_wheel.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/windows/palette.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <glm/gtc/packing.hpp>
#include <imgui.h>

namespace tmt {

void MaterialEditor::on_inspect() {
    const NodeHierarchy& hierarchy = editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>();

    const auto selected_entity = hierarchy.get_first_selected_entity();
    if (selected_entity == entt::null) {
        ImGui::TextWrapped("No voxel model selected.");
        return;
    }

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;

    Palette& palette = editor.systems[Editor::Mode::VOXEL].get<Palette>();
    Material& material = renderer->resource->blas->palette.entries[palette.get_selected_material_index()];

    ImGui::BeginGroup();

    static glm::vec3 albedo {};
    if (palette.update_material_editor) {
        albedo = material.albedo.unpack();
        palette.update_material_editor = false;
    }
    // We make sure to clamp the Rgb10 color to the range 0.0f-1.0f
    if (ImGui::ColorWheel3("Albedo", &albedo.x, ImGuiColorWheelFlags_CustomColorSpace)) material.albedo = Rgb10 { glm::clamp(albedo, glm::zero<glm::vec3>(), glm::one<glm::vec3>()) };

    float roughness = static_cast<float>(material.roughness) / 255.0f;
    if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) material.roughness = static_cast<uint8_t>(roughness * 255.0f);

    float metallic = static_cast<float>(material.metallic) / 255.0f;
    if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) material.metallic = static_cast<uint8_t>(metallic * 255.0f);

    float ior = glm::unpackHalf1x16(material.ior);
    if (ImGui::SliderFloat("IOR", &ior, 1.0f, 3.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) material.ior = glm::packHalf1x16(ior);

    float transmission = static_cast<float>(material.transmission) / 255.0f;
    if (ImGui::SliderFloat("Transmission", &transmission, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) material.transmission = static_cast<uint8_t>(transmission * 255.0f);

    float emission = glm::unpackHalf1x16(material.emission);
    if (ImGui::DragFloat("Emission", &emission, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_ClampOnInput)) material.emission = glm::packHalf1x16(emission);

    if (ImGui::BeginCombo("Type", magic_enum::enum_name(material.type).data())) {
        for (const auto& [value, name] : magic_enum::enum_entries<Material::Type>()) {
            if (!ImGui::Selectable(name.data(), material.type == value)) continue;

            material.type = value;
        }

        ImGui::EndCombo();
    }

    ImGui::EndGroup();

    if (ImGui::IsItemEdited()) renderer->resource->set_dirty();
}

}  // namespace tmt