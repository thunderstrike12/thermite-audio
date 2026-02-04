#pragma once

#include <graphite/resources/handle.hh>

#include "engine/shared/bvh2.hpp"

class RenderGraph;

namespace tmt {

struct RenderView;

/* Maximum number of voxel objects we support in the scene at once. */
constexpr uint32_t MAX_VOXEL_OBJECTS = 10000u;
/* We have a hard limit due to technical limitations. */
static_assert(MAX_VOXEL_OBJECTS < (1u << 14));
/* Maximum number of lights we support in the scene at once. */
constexpr uint32_t MAX_LIGHTS = 1000u;

struct GpuSceneView {
    uint32_t light_count = 0u;
};

/* View of the scene, containing all scene data required for rendering. */
struct SceneView {
    SceneView() = default;
    ~SceneView() = default;

    void init();
    void update(RenderGraph& render_graph, const RenderView& render_view);
    void deinit();

    /* GPU resources */
    Buffer bvh_nodes {};
    Buffer object_indices {};
    Buffer object_data {};
    Buffer lights_data {};
    Buffer scene_view {};

    /* Acceleration structures */
    Bvh2<struct VoxelObject> bvh {};
    std::vector<Entity> entities {};

   private:
    /* Collect and upload all voxel objects in the scene. */
    void update_voxel_objects(RenderGraph& render_graph);

    /* Collect and upload all lights in the scene. */
    void update_lights(RenderGraph& render_graph, const RenderView& render_view);
};

}  // namespace tmt
