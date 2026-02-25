#include "voxel_scene.hpp"

#include <graphite/vram_bank.hh>

#include "engine/tools/vengi_parser.hpp"
#include "engine/core/logger.hpp"
#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/tools/profiler.hpp"
#include "engine/tools/timer.hpp"
#include "engine/tools/svh_format.hpp"
#include "engine/tools/uuid.hpp"
#include "engine/shared/colorspace.hpp"

#include <queue>
#include <omp.h>

namespace {

void recurse_get_uuids(const tmt::VoxelSceneNode& node, std::vector<tmt::UUID>& uuids) {
    uuids.push_back(node.uuid);
    for (auto& children : node.children) {
        recurse_get_uuids(children, uuids);
    }
}

}  // namespace

namespace tmt {

void compute_physics_data(RawVoxels& voxels) {
    TMT_ZONE_SCOPED

    // More conservative pre-allocation estimate for better cache locality
    const size_t estimated_queue_size = static_cast<size_t>(std::pow(voxels.w * voxels.h * voxels.d, 2.0 / 3.0) * 3.0);

    std::vector<uint32_t> queue_vec;
    queue_vec.reserve(estimated_queue_size);

    // Phase 1: Classify voxels and compute normals (PARALLELIZED)
    {
        TMT_ZONE_SCOPED_N("VoxelClassification")

        // Thread-local queues for parallel collection
        const int num_threads = omp_get_max_threads();
        std::vector<std::vector<uint32_t>> thread_queues(num_threads);

        // Pre-allocate thread-local queues
        for (int i = 0; i < num_threads; i++) {
            thread_queues[i].reserve(estimated_queue_size / num_threads);
        }

#pragma omp parallel
        {
            const int thread_id = omp_get_thread_num();
            std::vector<uint32_t>& local_queue = thread_queues[thread_id];

#pragma omp for collapse(3) schedule(dynamic, 16)
            for (uint32_t z = 0; z < voxels.d; z++) {
                for (uint32_t y = 0; y < voxels.h; y++) {
                    for (uint32_t x = 0; x < voxels.w; x++) {
                        const uint32_t index = x + voxels.w * y + voxels.w * voxels.h * z;
                        if (voxels.physics_data[index].type == PhysicsVoxelType::EMPTY) continue;

                        // Check neighbors and compute normal
                        {
                            TMT_ZONE_SCOPED_N("NeighborAnalysis")

                            int empty_sides = 0;
                            glm::ivec3 normal = {};

                            // Precompute dimensions for bounds checking
                            const uint32_t w = voxels.w;
                            const uint32_t h = voxels.h;
                            const uint32_t d = voxels.d;
                            const uint32_t wh = w * h;

                            // Unrolled neighbor checking for better performance
                            // Check -X
                            if (x > 0) {
                                if (voxels.physics_data[index - 1].type == PhysicsVoxelType::EMPTY) {
                                    normal.x -= 1;
                                    empty_sides++;
                                }
                            } else {
                                normal.x -= 1;
                                empty_sides++;
                            }

                            // Check +X
                            if (x < w - 1) {
                                if (voxels.physics_data[index + 1].type == PhysicsVoxelType::EMPTY) {
                                    normal.x += 1;
                                    empty_sides++;
                                }
                            } else {
                                normal.x += 1;
                                empty_sides++;
                            }

                            // Check -Y
                            if (y > 0) {
                                if (voxels.physics_data[index - w].type == PhysicsVoxelType::EMPTY) {
                                    normal.y -= 1;
                                    empty_sides++;
                                }
                            } else {
                                normal.y -= 1;
                                empty_sides++;
                            }

                            // Check +Y
                            if (y < h - 1) {
                                if (voxels.physics_data[index + w].type == PhysicsVoxelType::EMPTY) {
                                    normal.y += 1;
                                    empty_sides++;
                                }
                            } else {
                                normal.y += 1;
                                empty_sides++;
                            }

                            // Check -Z
                            if (z > 0) {
                                if (voxels.physics_data[index - wh].type == PhysicsVoxelType::EMPTY) {
                                    normal.z -= 1;
                                    empty_sides++;
                                }
                            } else {
                                normal.z -= 1;
                                empty_sides++;
                            }

                            // Check +Z
                            if (z < d - 1) {
                                if (voxels.physics_data[index + wh].type == PhysicsVoxelType::EMPTY) {
                                    normal.z += 1;
                                    empty_sides++;
                                }
                            } else {
                                normal.z += 1;
                                empty_sides++;
                            }

                            voxels.physics_data[index].normal_index = 0;

                            // Classify voxel type based on empty neighbors
                            if (empty_sides == 0) {
                                voxels.physics_data[index].type = PhysicsVoxelType::INSIDE;
                            } else if (empty_sides == 1) {
                                TMT_ZONE_SCOPED_N("FaceNormalLookup")
                                for (size_t i = 1; i < 7; i++) {
                                    if (NORMAL_LUT[i] == normal) {
                                        voxels.physics_data[index].normal_index = i;
                                        break;
                                    }
                                }
                                voxels.physics_data[index].type = PhysicsVoxelType::FACE;
                            } else if (empty_sides == 2) {
                                TMT_ZONE_SCOPED_N("EdgeNormalLookup")
                                for (size_t i = 7; i < 19; i++) {
                                    if (NORMAL_LUT[i] == normal) {
                                        voxels.physics_data[index].normal_index = i;
                                        break;
                                    }
                                }
                                voxels.physics_data[index].type = PhysicsVoxelType::EDGE;
                            } else {  // empty_sides >= 3
                                TMT_ZONE_SCOPED_N("CornerNormalLookup")
                                for (size_t i = 19; i < 27; i++) {
                                    if (NORMAL_LUT[i] == normal) {
                                        voxels.physics_data[index].normal_index = i;
                                        break;
                                    }
                                }
                                voxels.physics_data[index].type = PhysicsVoxelType::CORNER;
                            }

                            if (voxels.physics_data[index].normal_index == 0 && empty_sides != 0) voxels.physics_data[index].normal_index = INVALID_NORMAL_INDEX;  // None empty 0 normal

                            if (voxels.physics_data[index].normal_index != 0) {
                                local_queue.push_back(index);
                            }
                        }
                    }
                }
            }
        }

        // Merge thread-local queues into main queue
        {
            TMT_ZONE_SCOPED_N("MergeQueues")
            size_t total_size = 0;
            for (const auto& q : thread_queues) {
                total_size += q.size();
            }
            queue_vec.reserve(total_size);

            for (auto& q : thread_queues) {
                queue_vec.insert(queue_vec.end(), q.begin(), q.end());
            }
        }
    }

    // Phase 2: Propagate normal indices via BFS (Sequential - inherently dependent)
    {
        TMT_ZONE_SCOPED_N("NormalPropagation")

        // Precompute dimensions for faster access
        const uint32_t w = voxels.w;
        const uint32_t h = voxels.h;
        const uint32_t d = voxels.d;
        const uint32_t wh = w * h;

        // Use index-based iteration instead of queue operations
        size_t read_pos = 0;

        while (read_pos < queue_vec.size()) {
            const uint32_t current = queue_vec[read_pos++];

            // Decompose index into coordinates
            const uint32_t x = current % w;
            const uint32_t y = (current % wh) / w;
            const uint32_t z = current / wh;

            const uint32_t current_normal_index = voxels.physics_data[current].normal_index;

            // Unrolled neighbor checking for better performance
            // Check -X neighbor
            if (x > 0) {
                const uint32_t neighbour = current - 1;
                if (voxels.physics_data[neighbour].normal_index == 0) {
                    voxels.physics_data[neighbour].normal_index = current_normal_index;
                    queue_vec.push_back(neighbour);
                }
            }

            // Check +X neighbor
            if (x < w - 1) {
                const uint32_t neighbour = current + 1;
                if (voxels.physics_data[neighbour].normal_index == 0) {
                    voxels.physics_data[neighbour].normal_index = current_normal_index;
                    queue_vec.push_back(neighbour);
                }
            }

            // Check -Y neighbor
            if (y > 0) {
                const uint32_t neighbour = current - w;
                if (voxels.physics_data[neighbour].normal_index == 0) {
                    voxels.physics_data[neighbour].normal_index = current_normal_index;
                    queue_vec.push_back(neighbour);
                }
            }

            // Check +Y neighbor
            if (y < h - 1) {
                const uint32_t neighbour = current + w;
                if (voxels.physics_data[neighbour].normal_index == 0) {
                    voxels.physics_data[neighbour].normal_index = current_normal_index;
                    queue_vec.push_back(neighbour);
                }
            }

            // Check -Z neighbor
            if (z > 0) {
                const uint32_t neighbour = current - wh;
                if (voxels.physics_data[neighbour].normal_index == 0) {
                    voxels.physics_data[neighbour].normal_index = current_normal_index;
                    queue_vec.push_back(neighbour);
                }
            }

            // Check +Z neighbor
            if (z < d - 1) {
                const uint32_t neighbour = current + wh;
                if (voxels.physics_data[neighbour].normal_index == 0) {
                    voxels.physics_data[neighbour].normal_index = current_normal_index;
                    queue_vec.push_back(neighbour);
                }
            }
        }
    }
}

/* Gather all voxels inside a voxel model node. */
inline RawVoxels gather_voxels(const vengi::Node* file_node) {
    TMT_ZONE_SCOPED
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
                    voxels.physics_data[i] = { PhysicsVoxelType::EMPTY, 0 };
                } else {
                    voxels.materials[i] = voxel.color_index;
                    voxels.physics_data[i] = { PhysicsVoxelType::INSIDE, 0 };
                }
            }
        }
    }

    compute_physics_data(voxels);

    return voxels;
}

/* Parse the vengi scene hierarchy. */
VoxelSceneNode parse_hierarchy(const vengi::Node* file_node) {
    TMT_ZONE_SCOPED

    /* Create a new scene node */
    VoxelSceneNode node {};
    node.uuid = UUID(file_node->uuid[0], file_node->uuid[1]);
    node.name = file_node->name;
    node.transform = file_node->transform;

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
            glm::vec3 color;
            color.r = (float)palette[i].color.r * (1.0f / 255.0f);
            color.g = (float)palette[i].color.g * (1.0f / 255.0f);
            color.b = (float)palette[i].color.b * (1.0f / 255.0f);

            Material material;
            material.albedo = cs::r709_to_acescg(cs::linearize(color));

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
    TMT_ZONE_SCOPED

    const std::string& file_extension = file_location.relative_path.extension().generic_string();
    if (file_extension == ".svh") {
        root_nodes = decode_svh(IO::read_file(file_location));
        if (root_nodes.empty()) Log::error("Failed to load voxel scene from file: {}", file_location);

        return !root_nodes.empty();
    }

    /* Parse the vengi file */
    std::unique_ptr<vengi::Node> root = nullptr;
    root = VengiParser::load(file_location);
    if (!root) {
        Log::error("Failed to load voxel scene from file: {}", file_location);
        return false;
    }

    /* Traverse & parse the vengi scene */
    root_nodes.push_back(parse_hierarchy(root.get()));

    return true;
}

void VoxelScene::unload() {
    root_nodes.clear();
}

std::vector<UUID> VoxelScene::get_all_uuids() const {
    std::vector<UUID> uuids;

    for (auto& node : root_nodes) {
        recurse_get_uuids(node, uuids);
    }

    return uuids;
}

}  // namespace tmt