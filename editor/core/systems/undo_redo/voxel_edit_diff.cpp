#include "voxel_edit_diff.hpp"

#include "editor/windows/node_hierarchy.hpp"

#include <engine/engine.hpp>
#include <engine/core/resources/voxel_volume.hpp>
#include <engine/core/components/voxel_renderer.hpp>
#include <engine/tools/serializer/ecs.hpp>

#include "editor.hpp"

namespace tmt {

namespace {

//// Get the resource with the UUID that we saved (storing the uuid prevents unnecessary storing of model resources).
// Entity get_node_from_uuid(const UUID& uuid) {
//     const entt::basic_group group = engine.ecs.get_registry().group<NodeHierarchy::NodeUUID>();
//
//     for (const auto&& [entity, node_uuid] : group.each()) {
//         if (node_uuid.uuid == uuid) return entity;
//     }
//
//     return entt::null;  // Return empty resource meaning no node with that UUID exists or that node doesn't have a model.
// }

// Get the resource with the UUID that we saved (storing the uuid prevents unnecessary storing of model resources).
ResourceRef<VoxelVolume> get_node_model(const UUID& uuid) {
    const entt::basic_group group = engine.ecs.get_registry().group<VoxelRenderer>(entt::get<Transform>);

    for (const auto&& [entity, renderer, transform] : group.each()) {
        if (renderer.resource->uuid == uuid) return renderer.resource;
    }

    // Should never happen, the change should only be available when a model with that UUID is loaded.
    return {};
}

}  // namespace

void VoxelEditDiff::add_change(const glm::uvec3& coord, const MaterialIndex* before_material, const MaterialIndex* material_after) {
    const bool is_empty_before = (before_material == nullptr);
    before_change.positions.emplace_back(coord.x, coord.y, coord.z, is_empty_before);
    if (!is_empty_before) before_change.indices.push_back(*before_material);

    const bool is_empty_after = (material_after == nullptr);
    after_change.positions.emplace_back(coord.x, coord.y, coord.z, is_empty_after);
    if (!is_empty_after) after_change.indices.push_back(*material_after);
}

void VoxelEditDiff::commit(const std::string& message) {
    // Don't commit the diff if there are no changes.
    if (before_change.positions.empty() && after_change.positions.empty()) return;

    send_to_manager(std::move(*this), message);
}

void VoxelEditDiff::undo() {
    const ResourceRef<VoxelVolume> model = get_node_model(node_uuid);

    size_t next_voxel_index = 0;
    for (auto [x, y, z, is_empty] : before_change.positions) {
        if (is_empty)
            model->blas->remove_voxel(x, y, z);
        else
            model->blas->set_voxel(x, y, z, before_change.indices[next_voxel_index++]);
    }

    model->set_dirty();
}

void VoxelEditDiff::redo() {
    const ResourceRef<VoxelVolume> model = get_node_model(node_uuid);

    size_t next_voxel_index = 0;
    for (auto [x, y, z, is_empty] : after_change.positions) {
        if (is_empty)
            model->blas->remove_voxel(x, y, z);
        else
            model->blas->set_voxel(x, y, z, after_change.indices[next_voxel_index++]);
    }

    model->set_dirty();
}

VoxelNodeDiff::VoxelNodeDiff(const Entity node_entity, const bool is_add) : is_add { is_add } {
    recurse_parse_node_data(node_entity);

    entity_json = Serializer::serialize(node_entity, engine.ecs);

    for (const NodeData& data : node_data) {
        if (data.model) engine.ecs.add_component<VoxelRenderer>(data.entity_id).resource = data.model;
    }
}

void VoxelNodeDiff::undo() {
    if (is_add) {
        engine.ecs.destroy_entity(node_data[0].entity_id);
    } else {
        Entity root_entity;
        Serializer::deserialize(entity_json, root_entity, engine.ecs);

        const Transform& transform = engine.ecs.get_component<Transform>(root_entity);
        if (!transform.has_parent()) editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().root_entities.emplace(root_entity);

        for (const NodeData& data : node_data) {
            engine.ecs.add_component<NodeHierarchy::NodeUUID>(data.entity_id).uuid = data.uuid;

            if (data.model) {
                VoxelRenderer& renderer = engine.ecs.add_component<VoxelRenderer>(data.entity_id);
                renderer.resource = data.model;
            }
        }
    }
}

void VoxelNodeDiff::redo() {
    if (!is_add) {
        engine.ecs.destroy_entity(node_data[0].entity_id);
    } else {
        Entity root_entity;
        Serializer::deserialize(entity_json, root_entity, engine.ecs);

        const Transform& transform = engine.ecs.get_component<Transform>(root_entity);
        if (!transform.has_parent()) editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().root_entities.emplace(root_entity);

        for (const NodeData& data : node_data) {
            engine.ecs.add_component<NodeHierarchy::NodeUUID>(data.entity_id).uuid = data.uuid;

            if (data.model) {
                VoxelRenderer& renderer = engine.ecs.add_component<VoxelRenderer>(data.entity_id);
                renderer.resource = data.model;
            }
        }
    }
}

void VoxelNodeDiff::recurse_parse_node_data(Entity entity) {
    const auto& node_uuid = engine.ecs.get_component<NodeHierarchy::NodeUUID>(entity);
    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(entity);

    if (renderer != nullptr) {
        node_data.emplace_back(entity, node_uuid.uuid, renderer->resource);
        engine.ecs.remove_component<VoxelRenderer>(entity);  // We remove the renderer component here only to add it back later (so it won't be serialized).
    } else {
        node_data.emplace_back(entity, node_uuid.uuid);
    }

    const Transform& transform = engine.ecs.get_component<Transform>(entity);
    for (const Entity child : transform.get_children()) {
        recurse_parse_node_data(child);
    }
}

}  // namespace tmt
