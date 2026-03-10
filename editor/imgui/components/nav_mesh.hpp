#pragma once
#include <ImReflect.hpp>

#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "editor/imgui/types/glm.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char* name, tmt::NavMesh& value, ImSettings& settings, ImResponse& response) {
    // load rig model if not loaded and make field for file location
    ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);

    // if(ImGui::Button("Average normals"))
    //{
    //     value.average_neighbor_normals();
    // }
}