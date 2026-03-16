#include "voxel_edit_diff.hpp"

#include "editor/windows/node_hierarchy.hpp"

#include <engine/engine.hpp>
#include <engine/core/resources/voxel_volume.hpp>
#include <engine/core/components/voxel_renderer.hpp>
#include <engine/tools/serializer/ecs.hpp>

#include "editor.hpp"

namespace tmt {

namespace {

// Get the resource with the UUID that we saved (storing the uuid prevents unnecessary storing of model resources).
Entity get_node_from_uuid(const UUID& uuid) {
    const entt::basic_group group = engine.ecs.group<NodeHierarchy::NodeUUID>();

    for (const auto&& [entity, node_uuid] : group.each()) {
        if (node_uuid.uuid == uuid) return entity;
    }

    return entt::null;  // Return empty resource meaning no node with that UUID exists or that node doesn't have a model.
}

// Get the resource with the UUID that we saved (storing the uuid prevents unnecessary storing of model resources).
ResourceRef<VoxelVolume> get_node_model(const UUID& uuid) {
    const entt::basic_group group = engine.ecs.group<VoxelRenderer>(entt::get<Transform>);

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
    UUID parent_uuid = NULL_UUID;

    const Entity parent_node = engine.ecs.get_component<Transform>(node_entity).get_parent();
    if (parent_node != entt::null) parent_uuid = engine.ecs.get_component<NodeHierarchy::NodeUUID>(parent_node).uuid;

    top_nodes_data.push_back(recurse_parse_node_data(node_entity, parent_uuid));
}

VoxelNodeDiff::VoxelNodeDiff(const std::span<const Entity>& node_entities, const bool is_add) : is_add { is_add } {
    for (const Entity node_entity : node_entities) {
        UUID parent_uuid = NULL_UUID;

        const Entity parent_node = engine.ecs.get_component<Transform>(node_entity).get_parent();
        if (parent_node != entt::null) parent_uuid = engine.ecs.get_component<NodeHierarchy::NodeUUID>(parent_node).uuid;

        top_nodes_data.push_back(recurse_parse_node_data(node_entity, parent_uuid));
    }
}

void VoxelNodeDiff::undo() {
    std::vector<Entity>& root_entities = editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().root_entities;

    if (is_add) {
        for (const NodeData& top_node_data : top_nodes_data) {
            const Entity entity = get_node_from_uuid(top_node_data.uuid);
            std::erase(root_entities, entity);  // Also erase the entity from the vector if it's a root entity, since we are deleting it.

            engine.ecs.destroy_entity(entity);
        }
    } else {
        for (const NodeData& top_node_data : top_nodes_data) {
            recurse_build_node_entities(top_node_data, root_entities);
        }
    }
}

void VoxelNodeDiff::redo() {
    std::vector<Entity>& root_entities = editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().root_entities;

    if (!is_add) {
        for (const NodeData& top_node_data : top_nodes_data) {
            const Entity entity = get_node_from_uuid(top_node_data.uuid);
            std::erase(root_entities, entity);  // Also erase the entity from the vector if it's a root entity, since we are deleting it.

            engine.ecs.destroy_entity(entity);
        }
    } else {
        for (const NodeData& top_node_data : top_nodes_data) {
            recurse_build_node_entities(top_node_data, root_entities);
        }
    }
}

void VoxelNodeDiff::recurse_build_node_entities(const NodeData& data, std::vector<Entity>& root_entities) {
    const Entity entity = engine.ecs.create_entity(data.name, data.old_id);

    auto&& [transform, uuid] = engine.ecs.add_or_get_component<Transform, NodeHierarchy::NodeUUID>(entity);
    uuid.uuid = data.uuid;

    if (data.parent_uuid == NULL_UUID)
        root_entities.push_back(entity);  // If the entity doesn't have parents it's a root entity, and we have to add it to the vector.
    else
        transform.set_parent(get_node_from_uuid(data.parent_uuid));

    transform.set_world_matrix(data.world_matrix);

    if (data.model) {
        VoxelRenderer& renderer = engine.ecs.add_component<VoxelRenderer>(entity);
        renderer.resource = data.model;
    }

    for (const NodeData& child_node : data.child_nodes) {
        recurse_build_node_entities(child_node, root_entities);
    }
}

VoxelNodeDiff::NodeData VoxelNodeDiff::recurse_parse_node_data(const Entity entity, const UUID& parent_uuid) {
    auto&& [name, transform, uuid] = engine.ecs.get_component<Name, Transform, NodeHierarchy::NodeUUID>(entity);

    NodeData data {
        .old_id = entity,
        .name = name.name,
        .uuid = uuid.uuid,
        .world_matrix = transform.get_world_matrix(),
        .parent_uuid = parent_uuid,
    };

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(entity);
    if (renderer != nullptr) data.model = renderer->resource;

    for (const Entity child : transform.get_children()) {
        data.child_nodes.push_back(recurse_parse_node_data(child, uuid.uuid));
    }

    return data;
}

void GridResizeDiff::undo() {
    if (before_resource == nullptr) {
        engine.ecs.remove_component<VoxelRenderer>(node_entity);
    } else {
        engine.ecs.add_or_get_component<VoxelRenderer>(node_entity).resource = before_resource;
    }

    Transform& transform = engine.ecs.get_component<Transform>(node_entity);
    transform.set_local_position(transform.get_local_position() - offset);

    for (const Entity child : transform.get_children()) {
        Transform& child_transform = engine.ecs.get_component<Transform>(child);
        child_transform.set_local_position(child_transform.get_local_position() + offset);
    }
}

void GridResizeDiff::redo() {
    if (after_resource == nullptr) {
        engine.ecs.remove_component<VoxelRenderer>(node_entity);
    } else {
        engine.ecs.add_or_get_component<VoxelRenderer>(node_entity).resource = after_resource;
    }

    Transform& transform = engine.ecs.get_component<Transform>(node_entity);
    transform.set_local_position(transform.get_local_position() + offset);

    for (const Entity child : transform.get_children()) {
        Transform& child_transform = engine.ecs.get_component<Transform>(child);
        child_transform.set_local_position(child_transform.get_local_position() - offset);
    }
}

void NodeVectorDiff::before() {
    before_uuids.reserve(container->size());
    for (const Entity entity : *container) {
        const UUID& uuid = engine.ecs.get_component<NodeHierarchy::NodeUUID>(entity).uuid;
        before_uuids.push_back(uuid);
    }
}

void NodeVectorDiff::after() {
    after_uuids.reserve(container->size());
    for (const Entity entity : *container) {
        const UUID& uuid = engine.ecs.get_component<NodeHierarchy::NodeUUID>(entity).uuid;
        after_uuids.push_back(uuid);
    }
}

void NodeVectorDiff::undo() {
    container->clear();

    container->reserve(before_uuids.size());
    for (const UUID& uuid : before_uuids) {
        container->push_back(get_node_from_uuid(uuid));
    }
}

void NodeVectorDiff::redo() {
    container->clear();

    container->reserve(after_uuids.size());
    for (const UUID& uuid : after_uuids) {
        container->push_back(get_node_from_uuid(uuid));
    }
}

}  // namespace tmt
