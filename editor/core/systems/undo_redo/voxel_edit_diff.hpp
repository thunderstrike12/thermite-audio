#pragma once

#include <engine/core/ecs.hpp>
#include <engine/shared/const.hpp>
#include <engine/core/renderer/material.hpp>
#include <engine/core/resources/voxel_volume.hpp>

#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"

namespace tmt {

class VoxelEditDiff : public IUndoRedo {
   public:
    VoxelEditDiff(const UUID& node_uuid) : node_uuid { node_uuid } {}

    // Add a change to a voxel, if voxel is/was empty, use `nullptr`.
    void add_change(const glm::uvec3& coord, const MaterialIndex* before_material, const MaterialIndex* material_after = nullptr);
    void commit(const std::string& message);

    // Inherited via IUndoRedo
    void undo() override;
    void redo() override;
    void inspect() override {}

   private:
    struct VoxelCoord {
        uint32_t x : 10 { 0 };
        uint32_t y : 10 { 0 };
        uint32_t z : 10 { 0 };
        uint32_t is_empty : 2 { false };  // Bool is uint32_t type to avoid padding in the struct.
    };

    struct VoxelChange {
        std::vector<VoxelCoord> positions;
        std::vector<MaterialIndex> indices;  // Only stores the indices of the voxels that aren't empty.
    };

    UUID node_uuid { NULL_UUID };

    VoxelChange before_change;
    VoxelChange after_change;
};

class VoxelNodeDiff : public IUndoRedo {
   public:
    VoxelNodeDiff(Entity node_entity, bool is_add);

    // Inherited via IUndoRedo
    void undo() override;
    void redo() override;
    void inspect() override {}

   private:
    struct NodeData {
        Entity entity_id;
        UUID uuid { NULL_UUID };
        ResourceRef<VoxelVolume> model { {}, nullptr };
    };

    void recurse_parse_node_data(Entity entity);

    bool is_add { false };

    json entity_json;
    std::vector<NodeData> node_data;
};

class GridResizeDiff : public IUndoRedo {
   public:
    // Before or after may be invalid resources, this implies the state before had no grid at all.
    GridResizeDiff(Entity node_entity, const glm::vec3& offset, const ResourceRef<VoxelVolume>& before, const ResourceRef<VoxelVolume>& after) :
        node_entity { node_entity }, offset { offset }, before_resource { before }, after_resource { after } {}

    // Inherited via IUndoRedo
    void undo() override;
    void redo() override;
    void inspect() override {}

   private:
    Entity node_entity;
    glm::vec3 offset {};

    ResourceRef<VoxelVolume> before_resource {};
    ResourceRef<VoxelVolume> after_resource {};
};

}  // namespace tmt