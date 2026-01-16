#pragma once

#include <graphite/resources/handle.hh>

#include "engine/shared/bvh2.hpp"

class RenderGraph;

namespace tmt {

/* Maximum number of voxel objects we support in the scene at once. */
constexpr uint32_t MAX_VOXEL_OBJECTS = 10000u;
/* We have a hard limit due to technical limitations. */
static_assert(MAX_VOXEL_OBJECTS < (1u << 14));

/* View of the scene, containing all scene data required for rendering. */
struct SceneView {
    SceneView() = default;
    ~SceneView() = default;

    void init();
    void update(RenderGraph& render_graph);
    void deinit();

    /* GPU resources */
    Buffer bvh_nodes {};
    Buffer object_indices {};
    Buffer object_data {};

    /* Acceleration structures */
    Bvh2<struct VoxelObject> bvh {};
    std::vector<Entity> entities {};
};

}  // namespace tmt
