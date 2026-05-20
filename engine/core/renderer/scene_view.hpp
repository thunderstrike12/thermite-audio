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
constexpr uint32_t MAX_LIGHTS = 1024u;

namespace light_grid {

constexpr uint32_t MAX_LIGHTS_PER_CELL = 64u;
constexpr uint32_t MAX_CASCADES = 6u;
constexpr uint32_t CASCADES_RESOLUTION = 16u;
constexpr uint32_t BITMASKS_PER_CASCADE = (MAX_LIGHTS + 31u) / 32u;                      /* div up */
constexpr uint32_t FIRST_CASCADE_BOUNDS = 32u;                                           /* world space units */
constexpr uint32_t FIRST_CASCADE_CELL_SIZE = FIRST_CASCADE_BOUNDS / CASCADES_RESOLUTION; /* world space units */

constexpr uint32_t CELLS_PER_CASCADE = CASCADES_RESOLUTION * CASCADES_RESOLUTION * CASCADES_RESOLUTION;
constexpr uint32_t TOTAL_LIGHTS_PER_CASCADE = MAX_LIGHTS_PER_CELL * CELLS_PER_CASCADE;
constexpr uint32_t LIGHT_GRID_BUFFER_SIZE = CELLS_PER_CASCADE * MAX_CASCADES * MAX_LIGHTS_PER_CELL;

}  // namespace light_grid

struct GpuSceneView {
    /* Direction pointing towards the sun. */
    glm::vec3 sun_dir {};
    /* Radius as cos(theta) of the sun, defines the softness of its shadows. */
    float sun_angle = 0.0f;
    /* Exitent luminance of the light source. (in cd/m2, in ACEScg color-space) */
    glm::vec3 sun_luminance {};
    /* Number of active lights in the scene. */
    glm::uint light_count = 0u;
    /* Environment map (full) bindless handle. */
    glm::uint envmap_full_handle = 0u;
    /* Environment map (filtered) bindless handle. */
    glm::uint envmap_filtered_handle = 0u;
    /* Color grading lut bindless handle. */
    glm::uint lut_handle = 0u;
};

/* View of the scene, containing all scene data required for rendering. */
struct SceneView {
    SceneView() = default;
    ~SceneView() = default;

    void init();
    void update(RenderGraph& render_graph, const RenderView& render_view);
    void deinit();

    /* Stats */
    uint32_t object_count = 0u;
    uint32_t light_count = 0u;
    bool sun_light_active = false;

    /* GPU resources */
    Buffer bvh_nodes {};
    Buffer object_indices {};
    Buffer object_data {};
    Buffer lights_data {};
    Buffer scene_view {};

    /* Acceleration structures */
    Bvh2<struct VoxelObject> bvh {};
    std::vector<uint32_t> uuid_free_list {};
    uint32_t next_uuid = 1u;
    std::vector<Entity> entities {};
    bool render_outlines = false;

    /* Light acc structure */
    Buffer cascades_bitmasks {}; /* Filled on the CPU. Stores light bitmask per cascade. */
    Buffer light_grid {};        /* Filled on the GPU, using the cascades_bitmasks. Stores light indices per cascade cell. */
    glm::vec3 light_grid_center = { 0.0f, 0.0f, 0.0f };
    bool update_light_grid_center = true;

    /* Any objects before this distance will be fully opaque. */
    float object_opaque_distance = 128.0f;
    /* Any objects after this distance will be fully transparent. */
    float object_transparent_distance = 152.0f;
    /* Factor to change the interpolation curve when going from opaque to transparent. */
    float object_opacity_transition = 10.0f;

    /* Fog options */
    float fog_anisotropy = 0.6f;
    float fog_scatter_strength = 0.25f;
    glm::vec3 fog_absorption = glm::vec3(0.02f);
    float particle_reflectance = 0.01f;

    /* Color grading LUT info */
    float lut_size = 0.0f;

   private:
    /* Collect and upload all voxel objects in the scene. */
    void update_voxel_objects(RenderGraph& render_graph);

    /* Collect and upload all lights in the scene. */
    void update_lights(RenderGraph& render_graph, const RenderView& render_view);

    /* EnTT Subscribe Event Callback */
    void on_renderer_destroyed(entt::registry& registry, entt::entity entity);

    /* Track Prev Frame Transform */
    std::unordered_map<Entity, glm::mat4> prev_transforms {};
};

}  // namespace tmt
