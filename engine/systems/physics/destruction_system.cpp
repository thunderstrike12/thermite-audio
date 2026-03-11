#include "destruction_system.hpp"
#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/physics/physics_system.hpp"

namespace tmt {

void Destruction::on_start() {
    Log::info("Destruction on_start");
}

void Destruction::on_update(const FrameData& /*time*/) {}

void Destruction::on_fixed_update(const FrameData& /*time*/) {}

void Destruction::on_end() {
    Log::info("Destruction on_end");
}

void Destruction::destroy_voxels(Entity entity, const Stencil* stencil, glm::ivec3 offset) {
    // Get voxel body component
    VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(entity);
    if (renderer == nullptr) return;

    // Subtract voxels from BLAS
    auto* resource = renderer->resource.resource.get();
    resource->blas.get()->subtract(stencil, offset);
    resource->set_dirty();

    // Find where the objects seperate and creates new entities for each part
    std::vector<Entity> seperated_entities = find_seperations(entity, stencil, offset);

    // Recalculate physics data for all seperate voxel bodies
    for (Entity seperate_entity : seperated_entities) {
        VoxelBody& seperate_vb = engine.ecs.get_component<VoxelBody>(seperate_entity);
        auto& volume = engine.ecs.get_component<VoxelRenderer>(seperate_entity).resource;
        Physics::recalculate_physics_data(seperate_vb, *volume.resource);
    }
}

std::vector<Entity> Destruction::find_seperations(Entity entity, const Stencil* stencil, glm::ivec3 offset) {
    VoxelRenderer& vb = engine.ecs.get_component<VoxelRenderer>(entity);
    auto* resource = vb.resource.resource.get();
    Svt64* tree = resource->blas.get();

    const glm::ivec3 dirs[] = { { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 } };
    std::vector<glm::uvec3> edge_indices;

    // Loop over the stencil and find all voxels where destruction happened next to
    for (uint32_t local_z = 0; local_z < stencil->size.z; local_z++) {
        for (uint32_t local_y = 0; local_y < stencil->size.y; local_y++) {
            for (uint32_t local_x = 0; local_x < stencil->size.x; local_x++) {
                uint32_t i = local_x + local_y * stencil->size.x + local_z * stencil->size.x * stencil->size.y;

                // if this voxel is empty, skip it
                if (stencil->data[i] == 0) continue;
                glm::ivec3 tree_pos = glm::ivec3(local_x, local_y, local_z) + offset;

                // If the world position of this voxel is outside the bounds of the resource, skip it
                if (tree_pos.x < 0 || tree_pos.y < 0 || tree_pos.z < 0 || static_cast<uint32_t>(tree_pos.x) > resource->size.x - 1 ||
                    static_cast<uint32_t>(tree_pos.y) > resource->size.y - 1 || static_cast<uint32_t>(tree_pos.z) > resource->size.z - 1)
                    continue;

                const uint32_t edge_vals[] = { 0, stencil->size.x - 1, 0, stencil->size.y - 1, 0, stencil->size.z - 1 };
                const glm::ivec3 local_pos(local_x, local_y, local_z);

                for (int d = 0; d < 6; d++) {
                    // If we are on the edge of the stencil, we can directly mark the neighboring voxel
                    uint32_t local_axis_val = local_pos[d / 2];
                    if (local_axis_val == edge_vals[d]) {
                        glm::ivec3 neighbor_pos = local_pos + dirs[d] + offset;

                        // If the world position is outside the bounds of the resource, skip it
                        if (neighbor_pos.x < 0 || neighbor_pos.y < 0 || neighbor_pos.z < 0) continue;
                        if (static_cast<uint32_t>(neighbor_pos.x) >= resource->size.x || static_cast<uint32_t>(neighbor_pos.y) >= resource->size.y ||
                            static_cast<uint32_t>(neighbor_pos.z) >= resource->size.z)
                            continue;

                        // If this voxel is empty in the tree, skip it
                        if (tree->get_physics_voxel(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z) == nullptr) continue;

                        edge_indices.push_back((glm::uvec3)neighbor_pos);
                    } else {  // We are not on the edge of the stencil so only mark the neighboring voxel if its empty int the tree
                        glm::ivec3 neighbor_pos = local_pos + dirs[d] + offset;

                        // If the world position is outside the bounds of the resource, skip it
                        if (neighbor_pos.x < 0 || neighbor_pos.y < 0 || neighbor_pos.z < 0) continue;
                        if (static_cast<uint32_t>(neighbor_pos.x) >= resource->size.x || static_cast<uint32_t>(neighbor_pos.y) >= resource->size.y ||
                            static_cast<uint32_t>(neighbor_pos.z) >= resource->size.z)
                            continue;

                        // If this voxel is empty in the tree, skip it
                        if (tree->get_physics_voxel(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z) == nullptr) continue;

                        edge_indices.push_back((glm::uvec3)neighbor_pos);
                    }
                }
            }
        }
    }

    // Loop over all the edge indices and do a flood fill to find all connected voxels
    // Max size 1.07gb
    std::vector<uint8_t> flood_grid(resource->size.x * resource->size.y * resource->size.z, 0);

    uint8_t current_mark = 1;
    for (const glm::uvec3& edge_index : edge_indices) {
        uint32_t i = edge_index.x + edge_index.y * resource->size.x + edge_index.z * resource->size.x * resource->size.y;
        // If this voxel is already marked, skip it
        if (flood_grid[i] != 0) continue;

        // Do a flood fill from this voxel and mark all connected voxels with the same index
        std::vector<glm::uvec3> stack { edge_index };
        while (!stack.empty()) {
            // Get current voxel from stack and remove it
            glm::uvec3 current = stack.back();
            stack.pop_back();

            // Calculate index of this voxel in the flood grid
            uint32_t current_i = current.x + current.y * resource->size.x + current.z * resource->size.x * resource->size.y;

            // If this voxel is already marked, or it does not exist in the tree, skip it
            if (flood_grid[current_i] != 0 || tree->get_physics_voxel(current.x, current.y, current.z) == nullptr) continue;

            // Mark this voxel
            flood_grid[current_i] = current_mark;

            // Add neighbors to the stack
            for (int d = 0; d < 6; d++) {
                glm::ivec3 neighbor_pos = glm::ivec3(current) + dirs[d];
                // If the world position is outside the bounds of the resource, skip it
                if (neighbor_pos.x < 0 || neighbor_pos.y < 0 || neighbor_pos.z < 0) continue;
                if (static_cast<uint32_t>(neighbor_pos.x) >= resource->size.x || static_cast<uint32_t>(neighbor_pos.y) >= resource->size.y ||
                    static_cast<uint32_t>(neighbor_pos.z) >= resource->size.z)
                    continue;
                stack.push_back((glm::uvec3)neighbor_pos);
            }
        }
        current_mark++;
    }

    if (current_mark > 2) Log::info("Seperation Found!");

    return std::vector<Entity> { entity };

    // for (size_t i = 0; i < stencil_size; i++) {
    //     // if this voxel is empty, skip it
    //     if (stencil->data[i] == 0) continue;

    //    uint32_t local_x = i % stencil->size.x;
    //    uint32_t local_y = (i / stencil->size.x) % stencil->size.y;
    //    uint32_t local_z = i / (stencil->size.x * stencil->size.y);

    //
    //    edge_indices.push_back(i);
    //}
}

}  // namespace tmt