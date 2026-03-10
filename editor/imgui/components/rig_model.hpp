#pragma once
#include <ImReflect.hpp>

#include "engine/systems/animation/rig_model.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "editor/imgui/types/glm.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::RigModel& value, ImSettings&, ImResponse&) {
    // load rig model if not loaded and make field for file location
    if (!value.rig_is_loaded) {
        ImGui::Text("Rig not loaded");
        ImReflect::Input("Animation Resource", value.data);
        if (ImGui::Button("Load Rig")) {
            auto entity = tmt::engine.ecs.get_entity(value);
            value.init(value.data.file_location, entity);
        }
    }

    if (!value.vox_is_loaded && value.rig_is_loaded) {
        ImGui::Text("Voxel not loaded");
        ImReflect::Input("Voxel Resource", value.vox_path);
        if (ImGui::Button("Load Rig")) {
            value.attach_voxel_objects();
        }
    }

    if (!value.data) return;
    for (const auto& [location, animation_name] : value.data->animation_files) {
        ImGui::Text("Animation: %s", animation_name.c_str());
        if (ImGui::Button(("Play " + animation_name).c_str())) {
            value.play_animation(animation_name, 1.0f, true);
        }
    }
}