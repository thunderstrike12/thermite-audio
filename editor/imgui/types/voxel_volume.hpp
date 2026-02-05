#pragma once

#include <ImReflect.hpp>

#include "engine/core/resources/voxel_volume.hpp"

static void tag_invoke(ImReflect::ImInput_t, const char*, tmt::VoxelVolume& value, ImSettings&, ImResponse&) {
    ImGui::Text("Size:");

    ImGui::SameLine();

    ImGui::BeginDisabled();
    ImReflect::Input("", value.size);
    ImGui::EndDisabled();
}