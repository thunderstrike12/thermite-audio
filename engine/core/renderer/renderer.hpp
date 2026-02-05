#pragma once

#include <vector>

#include <glm/gtc/quaternion.hpp>

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
    STEPS,       /* Visualize visibility pass step count. (0..128) */
    VISIBILITY,  /* Visualize visibility buffer data. */
    DEPTH,       /* Visualize geometric depth data. (0..100) */
    NORMALS,     /* Visualize geometric normal data. */
    ALBEDO,      /* Visualize material albedo. */
    ILLUMINANCE, /* Visualize illuminance from the cache. */
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

    /* Display mode for debugging */
    DisplayMode display_mode = DisplayMode::DEFAULT;

    /* Pipelines */
    class GeometryPipeline& geometry_pipeline;
    class DiPipeline& di_pipeline;
    class PolylinePipeline& polyline_pipeline;
    class VfxPipeline& vfx_pipeline;

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

#ifdef THERMITE_EDITOR
    void set_imgui(ImGUI* imgui);
#endif
};

}  // namespace tmt
