#include "voxel_scene.hpp"

#include <graphite/vram_bank.hh>

#include "engine/tools/vengi_parser.hpp"
#include "engine/core/logger.hpp"
#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

/* Gather all voxels inside a voxel model node. */
inline RawVoxels gather_voxels(const vengi::Node* file_node) {
    const vengi::Region& region = file_node->voxel_data->region;

    /* Create a new collection of raw voxels */
    RawVoxels voxels {};
    voxels.w = region.width();
    voxels.h = region.height();
    voxels.d = region.depth();

    /* Gather all voxels from the file node */
    voxels.voxels.resize(voxels.w * voxels.h * voxels.d);
    for (int32_t z = region.lower.z; z < region.upper.z; ++z) {
        for (int32_t y = region.lower.y; y < region.upper.y; ++y) {
            for (int32_t x = region.lower.x; x < region.upper.x; ++x) {
                const uint32_t i = (z - region.lower.z) * region.height() * region.width() + (y - region.lower.y) * region.width() + (x - region.lower.x);
                const vengi::VoxelInformation& voxel = file_node->voxel_data->voxels[i];
                if (voxel.is_air) {
                    voxels.voxels[i] = AIR_INDEX;
                } else {
                    voxels.voxels[i] = voxel.color_index;
                }
            }
        }
    }

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

    /* Traverse & parse the vengi scene */
    hierarchy = parse_hierarchy(root.get());
    return true;
}

void VoxelScene::unload() { hierarchy = {}; }

}  // namespace tmt
