#pragma once

#include "render_view.hpp"
#include "scene_view.hpp"

class GPUAdapter;
class RenderGraph;
class VRAMBank;
class ImGUI;

namespace tmt {

/* Renderer display mode. */
enum class DisplayMode : uint32_t {
    DEFAULT = 0u,
    STEPS,         /* Visualize visibility pass step count. (0..128) */
    VISIBILITY,    /* Visualize visibility buffer data. */
    DEPTH,         /* Visualize geometric depth data. (0..100) */
    NORMALS,       /* Visualize geometric normal data. */
    ALBEDO,        /* Visualize material albedo. */
    ILLUMINANCE,   /* Visualize illuminance from the cache. */
    CACHE,         /* Visualize cache info. */
    MOTIONVECTORS, /* Visualize motion vectors. */
};

/* Shading rate. */
enum class ShadingRate : uint32_t {
    FULL_RATE = 0u,    /* Perform shading for every pixel on screen. */
    HALF_RATE = 1u,    /* Perform shading for half the pixels on screen. */
    QUARTER_RATE = 2u, /* Perform shading for 1/4th the pixels on screen. */
};

/* Get the resolution for a given shading rate. */
inline Size3D rated_resolution(Size3D base, ShadingRate rate) {
    if (rate == ShadingRate::HALF_RATE) {
        base.x = base.x >> 1;
    } else if (rate == ShadingRate::QUARTER_RATE) {
        base.x = base.x >> 1;
        base.y = base.y >> 1;
    }
    return base;
}

struct RendererSettings {
    float bloom_radius = 0.2f;
    float bloom_threshold = 1.0f;
    ShadingRate diff_shading_rate = ShadingRate::HALF_RATE;
    ShadingRate spec_shading_rate = ShadingRate::QUARTER_RATE;
};

class Renderer {
    GPUAdapter& gpu;
    RenderGraph& render_graph;

    /* Debug camera */
    Transform debug_transform;
    Camera debug_camera;

   public:
    /* Rendering views */
    RenderView render_view {};
    SceneView scene_view {};

    Sampler linear_sampler {};
    Sampler point_sampler {};

    /* Display mode for debugging */
    DisplayMode display_mode = DisplayMode::DEFAULT;
    bool enable_taa = true;
    bool kill_particles = false;
    float bloom_radius = 0.25f;

    /* Pipelines */
    class GeometryPipeline& geometry_pipeline;
    class DiPipeline& di_pipeline;
    class PolylinePipeline& polyline_pipeline;
    class VfxPipeline& vfx_pipeline;
    class UiPipeline& ui_pipeline;
    class PostProcessPipeline& post_process_pipeline;

#ifdef THERMITE_EDITOR
    ImGUI* imgui = nullptr;
#endif

    Renderer();
    ~Renderer();

    /* Copy */
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void update();
    void end();

    Hit trace_ray(const Ray& ray) const;

    /* Get a reference to the primary vram bank, for resource creation. */
    VRAMBank& vram_bank();

    /* Destroy a resource for the primary render graph. */
    void destroy(OpaqueHandle& handle);

    Transform& get_debug_transform() { return debug_transform; }
    Camera& get_debug_camera() { return debug_camera; }

    glm::uvec2 viewport_size() const { return render_view.gpu_view.resolution; }

#ifdef THERMITE_EDITOR
    void set_imgui(ImGUI* imgui);
#endif
};

}  // namespace tmt

TMT_OBJECT(tmt::RendererSettings, (bloom_radius));