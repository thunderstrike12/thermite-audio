#include "voxel_scene.hpp"

#include <graphite/vram_bank.hh>

#include "engine/tools/vengi_parser.hpp"
#include "engine/core/logger.hpp"
#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"

#include <queue>

namespace tmt {

void compute_physics_data(RawVoxels& voxels) {
    // Precompute physics voxel data
    std::queue<uint32_t> queue {};

    for (uint32_t z = 0; z < voxels.d; z++) {
        for (uint32_t y = 0; y < voxels.h; y++) {
            for (uint32_t x = 0; x < voxels.w; x++) {
                const uint32_t index = x + voxels.w * y + voxels.w * voxels.h * z;
                if (voxels.physics_data[index].type == PhysicsVoxelType::EMPTY) continue;

                int empty_sides = 0;
                glm::ivec3 normal = {};
                for (size_t sign = 0; sign < 2; sign++) {
                    for (size_t dir = 0; dir < 3; dir++) {
                        glm::uvec3 temp = {x, y, z};
                        temp[(int)dir] += sign == 0 ? (int)1 : -((int)1);
                        const uint32_t temp_index = temp.x + voxels.w * temp.y + voxels.w * voxels.h * temp.z;

                        // Check if temp is outside of size or empty
                        if (!(temp.x >= 0 && temp.x < voxels.w && temp.y >= 0 && temp.y < voxels.h && temp.z >= 0 && temp.z < voxels.d) ||
                            voxels.physics_data[temp_index].type == PhysicsVoxelType::EMPTY) {
                            normal[(int)dir] += sign == 0 ? 1 : -1;
                            empty_sides++;
                        }
                    }
                }

                voxels.physics_data[index].normal_index = 0;

                if (empty_sides == 0)
                    voxels.physics_data[index].type = PhysicsVoxelType::INSIDE;
                else if (empty_sides == 1) {
                    for (size_t i = 1; i < 7; i++) {
                        if (NORMAL_LUT[i] == normal) voxels.physics_data[index].normal_index = i;
                    }
                    voxels.physics_data[index].type = PhysicsVoxelType::FACE;
                } else if (empty_sides == 2) {
                    for (size_t i = 7; i < 19; i++) {
                        if (NORMAL_LUT[i] == normal) voxels.physics_data[index].normal_index = i;
                    }
                    voxels.physics_data[index].type = PhysicsVoxelType::EDGE;
                } else if (empty_sides >= 3) {
                    for (size_t i = 19; i < 27; i++) {
                        if (NORMAL_LUT[i] == normal) voxels.physics_data[index].normal_index = i;
                    }
                    voxels.physics_data[index].type = PhysicsVoxelType::CORNER;
                }

                if (voxels.physics_data[index].normal_index != 0) queue.push(index);
            }
        }
    }

    while (!queue.empty()) {
        const uint32_t& current = queue.front();
        uint32_t x = current % voxels.w;
        uint32_t y = (current % (voxels.w * voxels.h)) / voxels.w;
        uint32_t z = current / (voxels.w * voxels.h);

        // add voxel neighbours to queue and propegate normal index
        for (int sign = 0; sign < 2; sign++) {
            for (size_t axis = 0; axis < 3; axis++) {
                glm::uvec3 p = glm::uvec3(x, y, z);
                p[(glm::uvec3::length_type)axis] += (sign * 2) - 1;

                // If not in range continue
                if (p.x >= voxels.w || p.y >= voxels.h || p.z >= voxels.d) continue;

                // If the voxel already has a normal index continue
                uint32_t neighbour = p.x + voxels.w * p.y + voxels.w * voxels.h * p.z;
                if (voxels.physics_data[neighbour].normal_index != 0) continue;

                voxels.physics_data[neighbour].normal_index = voxels.physics_data[current].normal_index;

                queue.push(neighbour);
            }
        }

        queue.pop();
    }
}

/* Gather all voxels inside a voxel model node. */
inline RawVoxels gather_voxels(const vengi::Node* file_node) {
    const vengi::Region& region = file_node->voxel_data->region;

    /* Create a new collection of raw voxels */
    RawVoxels voxels {};
    voxels.w = region.width();
    voxels.h = region.height();
    voxels.d = region.depth();

    /* Safety check */
    assert(voxels.w <= 1024u && voxels.h <= 1024u && voxels.d <= 1024u && "models cannot be larger than 1024 on any axis.");

    /* Gather all voxels from the file node */
    voxels.materials.resize(voxels.w * voxels.h * voxels.d);
    voxels.physics_data.resize(voxels.w * voxels.h * voxels.d);
    for (int32_t z = region.lower.z; z < region.upper.z; ++z) {
        for (int32_t y = region.lower.y; y < region.upper.y; ++y) {
            for (int32_t x = region.lower.x; x < region.upper.x; ++x) {
                const uint32_t i = (z - region.lower.z) * region.height() * region.width() + (y - region.lower.y) * region.width() + (x - region.lower.x);
                const vengi::VoxelInformation& voxel = file_node->voxel_data->voxels[i];
                if (voxel.is_air) {
                    voxels.materials[i] = AIR_INDEX;
                    voxels.physics_data[i] = {PhysicsVoxelType::EMPTY, 0};
                } else {
                    voxels.materials[i] = voxel.color_index;
                    voxels.physics_data[i] = {PhysicsVoxelType::INSIDE, 0};
                }
            }
        }
    }

    compute_physics_data(voxels);

    return voxels;
}

/* Parse the vengi scene hierarchy. */
VoxelSceneNode parse_hierarchy(const vengi::Node* file_node) {
    /* Create a new scene node */
    VoxelSceneNode node {};
    node.uuid[0] = file_node->uuid[0];
    node.uuid[1] = file_node->uuid[1];

    /* If this node is a voxel model node */
    if (file_node->type == vengi::NodeType::MODEL) {
        /* Collect raw voxel data */
        const RawVoxels raw_voxels = gather_voxels(file_node);
        node.size = glm::uvec3(raw_voxels.w, raw_voxels.h, raw_voxels.d);

        /* Create and build the voxel acceleration structure */
        node.tree = std::make_unique<Svt64>();
        node.tree->build(raw_voxels);

        /* Load the voxel material palette */
        const std::vector<vengi::PaletteColor>& palette = file_node->palette->colors;
        for (uint32_t i = 0u; i < palette.size(); ++i) {
            Material material {};
            material.albedo_r = (float)palette[i].color.r * (1.0f / 255.0f);
            material.albedo_g = (float)palette[i].color.g * (1.0f / 255.0f);
            material.albedo_b = (float)palette[i].color.b * (1.0f / 255.0f);
            node.tree->palette.entries[i] = material;
        }
    }

    /* Recursively traverse child nodes */
    node.children.resize(file_node->children.size());
    for (uint32_t i = 0u; i < file_node->children.size(); ++i) {
        node.children[i] = parse_hierarchy(file_node->children[i].get());
    }
    return node;
}

bool VoxelScene::load() {
    /* Parse the vengi file */
    const std::unique_ptr<vengi::Node> root = VengiParser::load(file_location);
    if (!root) {
        Log::error("Failed to load voxel scene from file: {}", file_location.get_relative_path().string());
        return false;
    }

    /* Traverse & parse the vengi scene */
    hierarchy = parse_hierarchy(root.get());
    return true;
}

void VoxelScene::unload() { hierarchy = {}; }

}  // namespace tmt
