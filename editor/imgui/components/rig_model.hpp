#pragma once
#include <ImReflect.hpp>

#include "engine/systems/animation/rig_model.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "editor/imgui/types/glm.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::RigModel& value, ImSettings&, ImResponse&) {
    // Load rig model if not loaded and make field for file location
    if (!value.data) {
        ImGui::Text("Rig not loaded");
        ImReflect::ImResponse response = ImReflect::Input("", value.data);
        const bool rig_changed = response.get<tmt::ResourceRef<tmt::RigData>>().is_changed();

        if (rig_changed && value.data) {
            const auto entity = tmt::engine.ecs.get_entity(value);
            value.init(value.data.file_location, entity);
        }

        return;
    }

    if (!value.vox_is_loaded) {
        ImGui::Text("Voxel objects not loaded.");
        ImReflect::Input("", value.vox_path);

        if (ImGui::Button("Attach Voxels")) {
            value.attach_voxel_objects();
        }
    }

    if (ImGui::Button("Reload Rig")) value.data->reload();

    ImGui::Text("Current Animation: ");
    ImGui::Indent();
    ImGui::Text("%s", value.current_animation_playing().c_str());
    ImGui::Unindent();

    ImGui::NewLine();

    ImGui::Text("Loaded Animations:");
    ImGui::Indent();

    auto& animation_files = value.data->animation_files;

    auto iterator = animation_files.end();
    for (auto& [location, animation_name] : animation_files) {
        ImGui::PushID(animation_name.c_str());

        if (ImGui::SmallButton(ICON_MS_DELETE)) iterator = animation_files.find(location);

        ImGui::SameLine();
        ImGui::Text("%s", animation_name.c_str());
        ImGui::SameLine();

        if (ImGui::SmallButton("Play")) value.play_animation(animation_name, 1.0f, true);

        ImGui::PopID();
    }
    ImGui::Unindent();

    if (iterator != animation_files.end()) {
        animation_files.erase(iterator);
        value.data->reload();
    }

    static tmt::ResourceRef<tmt::RigData> animation_file;
    if (ImGui::CollapsingHeader("Add Animation")) {
        ImReflect::Input("", animation_file);

        if (animation_file != nullptr && animation_file->is_loaded()) {
            const auto& bones = animation_file->bones;
            if (!bones.empty() && !bones.back().animations.empty()) {
                animation_files.emplace(animation_file.file_location, bones.back().animations.begin()->first);
                value.data->reload();
            }

            animation_file = {};  // Clear the resource.
        }
    }
}