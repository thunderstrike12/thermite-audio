#pragma once

#include <ImReflect.hpp>

#include "engine/engine.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "editor/imgui/types/io.hpp"

template <typename T>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::ResourceRef<T>& value, ImSettings& settings, ImResponse& response) {
    ImGui::BeginGroup();
    ImReflect::Input(label, value.file_location, settings, response);

    const bool dropped = response.get<tmt::IO::FileLocation>().is_file_dropped();

    if (ImGui::Button("Load") || dropped) {
        // TODO: Sven move this if-check to resources.hpp
        if constexpr (std::derived_from<T, tmt::FileResource>) {
            value.resource = tmt::engine.resources.load_resource<T>(value.file_location).resource;
        } else if constexpr (requires { typename T::is_runtime_resource; }) {
            value.resource = tmt::engine.resources.copy_resource<T>(value.file_location).resource;
        } else {
            static_assert(svh::always_false<T>::value, "JsonSerializer Error: Cannot deserialize ResourceRef<T> where T is not a FileResource or RuntimeResource");
        }
        response.get<tmt::ResourceRef<T>>().changed();
    }
    ImGui::EndGroup();

    if constexpr (ImReflect::Detail::has_imreflect_input_v<T>) {
        ImReflect::Input(label, value.resource, settings, response);
    }
}

inline void recurse_node_names(const tmt::VoxelSceneNode& node, tmt::ResourceRef<tmt::VoxelVolume>& value, tmt::UUID& new_uuid) {
    constexpr ImGuiTreeNodeFlags DEFAULT_FLAGS = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull;

    ImGuiTreeNodeFlags flags = DEFAULT_FLAGS;
    flags |= (node.children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);
    flags |= (node.uuid == value->uuid ? ImGuiTreeNodeFlags_Selected : 0);

    ImGui::BeginDisabled(node.tree == nullptr);
    const bool is_open = ImGui::TreeNodeEx(node.name.c_str(), flags);
    ImGui::EndDisabled();

    if (ImGui::IsItemClicked() && node.uuid != value->uuid) new_uuid = node.uuid;

    if (!is_open) return;

    for (const tmt::VoxelSceneNode& child_node : node.children) {
        recurse_node_names(child_node, value, new_uuid);
    }

    ImGui::TreePop();
}

template <>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::ResourceRef<tmt::VoxelVolume>& value, ImSettings& settings, ImResponse& response) {
    ImGui::BeginGroup();
    ImReflect::Input(label, value.file_location, settings, response);

    const bool dropped = response.get<tmt::IO::FileLocation>().is_file_dropped();

    if (value) {
        ImGui::Indent();

        tmt::UUID new_uuid = tmt::NULL_UUID;
        if (ImGui::BeginCombo("Node", value->name.c_str())) {
            for (const tmt::VoxelSceneNode& root_node : value->file_resource->root_nodes) {
                recurse_node_names(root_node, value, new_uuid);
            }

            ImGui::EndCombo();
        }
        if (new_uuid != tmt::NULL_UUID) value.resource = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(value.file_location, new_uuid).resource;

        ImGui::Unindent();
    }

    if (ImGui::Button("Load") || dropped) {
        value.resource = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(value.file_location).resource;
    }
    ImGui::EndGroup();

    ImReflect::Input(label, value.resource, settings, response);
}
