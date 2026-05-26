#include "material_editor.hpp"

#include "editor.hpp"
#define IM_CUSTOM_COLORSPACE_MATRIX { 0.613097f, 0.070194f, 0.020616f, 0.339523f, 0.916354f, 0.109570f, 0.047380f, 0.013452f, 0.869815f }
#include "editor/shared/imgui_color_wheel.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/windows/palette.hpp"
#include "editor/core/systems/undo_redo/voxel_edit_diff.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <glm/gtc/packing.hpp>
#include <imgui.h>

#include "core/systems/undo_redo/type_diff.hpp"

namespace {

template <typename Type>
void modify_all_palettes(const std::vector<tmt::MaterialIndex>& material_indices, Type tmt::Material::* member, const Type value) {
    const entt::basic_group group = tmt::engine.ecs.group<tmt::VoxelRenderer>(entt::get<tmt::Transform>);

    // Loop over every renderer in the scene and modify their palettes.
    for (const auto&& [entity, renderer, transform] : group.each()) {
        for (const tmt::MaterialIndex material_index : material_indices) {
            tmt::Material& material = renderer.resource->blas->palette.entries[material_index];

            // Set the value though member pointer (means I only had to create 1 function for all material members).
            material.*member = value;
        }

        // Make sure to set all of them dirty.
        renderer.resource->set_dirty();
    }
}

template <typename Type>
void set_material_property(tmt::MaterialPalette* material_palette, const std::vector<tmt::MaterialIndex>& material_indices, Type tmt::Material::* member, const Type value) {
    // Material palette might be nullptr, meaning that all palettes must be modified.
    if (material_palette == nullptr) {
        modify_all_palettes(material_indices, member, value);
        return;
    }

    for (const tmt::MaterialIndex material_index : material_indices) {
        tmt::Material& material = material_palette->entries[material_index];

        // Set the value though member pointer (means I only had to create 1 function for all material members).
        material.*member = value;
    }
}

template <typename Type>
void undo_material_member(Type tmt::Material::* member, const std::unordered_map<tmt::UUID, std::vector<Type>>& previous_values) {
    const tmt::Palette& palette = tmt::editor.systems[tmt::Editor::Mode::VOXEL].get<tmt::Palette>();
    const std::vector<tmt::MaterialIndex>& selected_material_indices = palette.get_selected_material_indices();

    const entt::basic_group group = tmt::engine.ecs.group<tmt::VoxelRenderer>(entt::get<tmt::Transform>);

    // Loop over every renderer in the scene and modify their palettes.
    for (const auto&& [entity, renderer, transform] : group.each()) {
        const auto value_iterator = previous_values.find(renderer.resource->uuid);

        if (value_iterator == previous_values.end()) continue;

        const std::vector<Type>& values = value_iterator->second;

        for (size_t i = 0; i < selected_material_indices.size(); i++) {
            tmt::Material& material = renderer.resource->blas->palette.entries[selected_material_indices[i]];

            material.*member = values[i];
        }

        // Make sure to set all of them dirty.
        renderer.resource->set_dirty();
    }
}

template <typename Type>
void redo_material_member(Type tmt::Material::* member, const Type value) {
    const tmt::Palette& palette = tmt::editor.systems[tmt::Editor::Mode::VOXEL].get<tmt::Palette>();
    const auto& selected_material_indices = palette.get_selected_material_indices();

    const tmt::MaterialEditor& material_editor = tmt::editor.systems[tmt::Editor::Mode::VOXEL].get<tmt::MaterialEditor>();
    if (material_editor.edit_all_palettes) {
        set_material_property(nullptr, selected_material_indices, member, value);
        return;
    }

    const tmt::NodeHierarchy& hierarchy = tmt::editor.systems[tmt::Editor::Mode::VOXEL].get<tmt::NodeHierarchy>();
    const auto selected_entity = hierarchy.get_first_selected_entity();
    if (selected_entity == entt::null) return;

    const tmt::VoxelRenderer* renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;

    set_material_property(&renderer->resource->blas->palette, selected_material_indices, member, value);
}

}  // namespace

namespace tmt {

void MaterialEditor::on_inspect() {
    {
        TypeDiff global_palette_diff { &edit_all_palettes };
        global_palette_diff.before();
        const bool toggled = ImGui::Checkbox("Modify all palettes", &edit_all_palettes);
        global_palette_diff.after();

        if (toggled) TypeDiff<bool>::send_to_manager(std::move(global_palette_diff), "Toggle global palette");
    }

    ImGui::Separator();

    const NodeHierarchy& hierarchy = editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>();

    const auto selected_entity = hierarchy.get_first_selected_entity();
    if (selected_entity == entt::null) {
        ImGui::TextWrapped("No voxel model selected.");
        return;
    }

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;
    MaterialPalette* palette_ptr = (edit_all_palettes ? nullptr : &renderer->resource->blas->palette);

    Palette& palette = editor.systems[Editor::Mode::VOXEL].get<Palette>();
    const auto& selected_material_indices = palette.get_selected_material_indices();

    const MaterialIndex material_index = selected_material_indices.front();
    const Material& material = renderer->resource->blas->palette.entries[material_index];

    ImGui::BeginGroup();

    static glm::vec3 albedo {};
    static glm::vec3 edge_tint {};
    if (palette.update_material_editor) {
        albedo = material.albedo.unpack();
        edge_tint = material.edge_tint.unpack();
        palette.update_material_editor = false;
    }

    {
        // We make sure to clamp the Rgb10 color to the range 0.0f-1.0f
        const bool albedo_modified = ImGui::ColorWheel3("Albedo", &albedo.x, ImGuiColorWheelFlags_CustomColorSpace);
        const Rgb10 albedo_value { glm::clamp(albedo, glm::zero<glm::vec3>(), glm::one<glm::vec3>()) };

        handle_material_change(selected_entity, selected_material_indices, &Material::albedo, albedo_value);
        if (albedo_modified) set_material_property(palette_ptr, selected_material_indices, &Material::albedo, albedo_value);
    }

    {
        // We make sure to clamp the Rgb10 color to the range 0.0f-1.0f
        const bool edge_tint_modified = ImGui::ColorWheel3("Edge Tint", &edge_tint.x, ImGuiColorWheelFlags_CustomColorSpace);
        const Rgb10 edge_tint_value { glm::clamp(edge_tint, glm::zero<glm::vec3>(), glm::one<glm::vec3>()) };

        handle_material_change(selected_entity, selected_material_indices, &Material::edge_tint, edge_tint_value);
        if (edge_tint_modified) set_material_property(palette_ptr, selected_material_indices, &Material::edge_tint, edge_tint_value);
    }

    {
        float roughness = static_cast<float>(material.roughness) / 255.0f;
        const bool roughness_modified = ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput);
        const uint8_t roughness_value = static_cast<uint8_t>(roughness * 255.0f);

        handle_material_change(selected_entity, selected_material_indices, &Material::roughness, roughness_value);
        if (roughness_modified) set_material_property(palette_ptr, selected_material_indices, &Material::roughness, roughness_value);
    }

    {
        float metallic = static_cast<float>(material.metallic) / 255.0f;
        const bool metallic_modified = ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput);
        const uint8_t metallic_value = static_cast<uint8_t>(metallic * 255.0f);

        handle_material_change(selected_entity, selected_material_indices, &Material::metallic, metallic_value);
        if (metallic_modified) set_material_property(palette_ptr, selected_material_indices, &Material::metallic, metallic_value);
    }

    {
        float ior = glm::unpackHalf1x16(material.ior);
        const bool ior_modified = ImGui::SliderFloat("IOR", &ior, 1.0f, 3.0f, "%.3f", ImGuiSliderFlags_ClampOnInput);
        const uint16_t ior_value = glm::packHalf1x16(ior);

        handle_material_change(selected_entity, selected_material_indices, &Material::ior, ior_value);
        if (ior_modified) set_material_property(palette_ptr, selected_material_indices, &Material::ior, ior_value);
    }

    {
        float transmission = static_cast<float>(material.transmission) / 255.0f;
        const bool transmission_modified = ImGui::SliderFloat("Transmission", &transmission, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput);
        const uint8_t transmission_value = static_cast<uint8_t>(transmission * 255.0f);

        handle_material_change(selected_entity, selected_material_indices, &Material::transmission, transmission_value);
        if (transmission_modified) set_material_property(palette_ptr, selected_material_indices, &Material::transmission, transmission_value);
    }

    {
        float emission = glm::unpackHalf1x16(material.emission);
        const bool emission_modified = ImGui::DragFloat("Emission", &emission, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_ClampOnInput);
        const uint16_t emission_value = glm::packHalf1x16(emission);

        handle_material_change(selected_entity, selected_material_indices, &Material::emission, emission_value);
        if (emission_modified) set_material_property(palette_ptr, selected_material_indices, &Material::emission, emission_value);
    }

    {
        if (ImGui::BeginCombo("Type", magic_enum::enum_name(material.type).data())) {
            for (const auto& [value, name] : magic_enum::enum_entries<Material::Type>()) {
                if (!ImGui::Selectable(name.data(), material.type == value)) continue;

                // Don't retrigger if the value that was clicked is the same.
                if (material.type == value) continue;

                handle_material_change(selected_entity, selected_material_indices, &Material::type, value, true);
                set_material_property(palette_ptr, selected_material_indices, &Material::type, value);
            }

            ImGui::EndCombo();
        }
    }

    ImGui::EndGroup();

    if (ImGui::IsItemEdited()) renderer->resource->set_dirty();
}

template <typename Type>
void MaterialEditor::handle_material_change(const Entity selected_entity, const std::vector<MaterialIndex>& material_indices, Type Material::* member, Type after_value, const bool activated) {
    if (ImGui::IsItemActivated() || activated) {
        diff = new NodePaletteDiff { material_indices };
        std::unordered_map<UUID, std::vector<Type>> previous_values;

        if (edit_all_palettes) {
            const entt::basic_group group = engine.ecs.group<VoxelRenderer>(entt::get<Transform>);

            // Loop over every renderer in the scene and modify their palettes.
            for (const auto&& [entity, renderer, transform] : group.each()) {
                std::vector<Type>& material_values = previous_values[renderer.resource->uuid];
                material_values.reserve(material_indices.size());

                for (const MaterialIndex material_index : material_indices) {
                    const Material& material = renderer.resource->blas->palette.entries[material_index];
                    material_values.push_back(material.*member);
                }
            }
        } else {
            auto&& [uuid, renderer] = engine.ecs.get_component<NodeHierarchy::NodeUUID, VoxelRenderer>(selected_entity);
            std::vector<Type>& material_values = previous_values[uuid.uuid];
            material_values.reserve(material_indices.size());

            for (const MaterialIndex material_index : material_indices) {
                Material& material = renderer.resource->blas->palette.entries[material_index];
                material_values.push_back(material.*member);
            }
        }

        diff->before([member, before_values = std::move(previous_values)] { undo_material_member(member, before_values); });
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        diff->after([member, after_value] { redo_material_member(member, after_value); });
        tmt::NodePaletteDiff::send_to_manager(std::move(*diff), "Modified material property(s)");
        delete diff;
    }
}

}  // namespace tmt