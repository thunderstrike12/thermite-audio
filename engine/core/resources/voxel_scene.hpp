#pragma once

#include "engine/core/entity.hpp"
#include "engine/core/resource.hpp"
#include "engine/shared/svt64.hpp"
#include "engine/core/reflection.hpp"
#include "engine/tools/uuid.hpp"
#include "engine/systems/physics/components/destructable.hpp"

namespace tmt {

/* Voxel scene resource node. */
struct VoxelSceneNode {
    /* 128 bit unique identifier. */
    UUID uuid { NULL_UUID };

    Destructible destructible {};

    /* Voxel acceleration structure. */
    std::unique_ptr<Svt64> tree {};
    glm::uvec3 size {};

    /* Child nodes. */
    std::vector<VoxelSceneNode> children {};

    /* Node name. */
    std::string name;

    /* Local node transform. */
    glm::mat4 transform {};

    /* Returns true if this node is a voxel volume. */
    inline bool is_volume() const { return tree != nullptr; }
};

VoxelSceneNode* find_model_by_uuid(VoxelSceneNode& node, const UUID& uuid);

/* Voxel scene resource, loaded from a `.vengi` file. */
class VoxelScene : public tmt::FileResource {
   public:
    VoxelScene(tmt::IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;

    std::vector<Entity> instantiate_entities(const tmt::Entity parent = entt::null, const bool in_world_space = true) const;
    [[nodiscard]] std::vector<UUID> get_all_uuids() const;
    /* Tries to calculate the world matrix of the node with the given UUID. */
    glm::mat4 get_node_world_matrix(const UUID& uuid) const;

    bool fallback(FallbackReason reason) override;

    inline static const std::set<std::string_view> SUPPORTED_FILE_EXTENSIONS { ".svh" };

    /* Voxel scene hierarchy. */
    std::vector<VoxelSceneNode> root_nodes {};
};

}  // namespace tmt
