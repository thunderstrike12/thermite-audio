#pragma once

#include "render_view.hpp"
#include "scene_view.hpp"

#include "engine/core/system.hpp"

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
    LIGHTS,        /* Visualize light cascades and heatmap. */
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
    /* Bloom settings */
    float bloom_radius = 0.5f;
    float bloom_threshold = 1.0f;
    float bloom_trail = 1.0f;

    /* Auto exposure settings */
    float autox_key_value = 0.01f;
    float autox_lum_min = 0.001f;
    float autox_lum_max = 100.0f;
    float autox_response = 1.0f;

    /* Fog/Volumetrics settings */
    float fog_base_step_size = 0.1f;
    uint32_t fog_step_count = 16u;

    /* Shading rate settings */
    ShadingRate diff_shading_rate = ShadingRate::HALF_RATE;
    ShadingRate spec_shading_rate = ShadingRate::QUARTER_RATE;
};

struct ScreenshotSettings {
    enum class CaptureResolution {
        MatchViewport,
        HD_720p,
        FullHD_1080p,
        QHD_1440p,
        UHD_4K,
        UHD_8K,
    } resolution_preset = CaptureResolution::MatchViewport;

    std::string directory = "screenshots";
    std::string filename = "unknown-screenshot";

    uint32_t width = 1920u;
    uint32_t height = 1080u;

    bool include_ui = true;
    bool request_capture = false;

    bool warming_up = true;
    uint32_t warmup_frames = 0u;
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

    /* Samplers */
    Sampler linear_sampler {};
    Sampler point_sampler {};
    Sampler down_sampler {};
    Sampler up_sampler {};

    /* Partial AutoX texture (256x256) */
    Texture autox_partial_texture {};
    Image autox_partial_image {};
    /* Tiny AutoX texture (16x16) */
    Texture autox_tiny_texture {};
    Image autox_tiny_image {};
    /* Final AutoX texture (1x1) */
    Texture autox_texture {};
    Image autox_image {};

    /* Display mode for debugging */
    DisplayMode display_mode = DisplayMode::DEFAULT;
    bool enable_taa = true;
    bool kill_particles = false;
    float bloom_radius = 0.25f;

    /* Screenshot settings */
    ScreenshotSettings screenshot_settings {};

    /* Pipelines */
    class GeometryPipeline& geometry_pipeline;
    class LightingPipeline& lighting_pipeline;
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

    void capture_screenshot();

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

class RendererSerializer : public ISystem {
   public:
    RendererSerializer() = default;
    ~RendererSerializer() = default;

    constexpr virtual std::string get_name() override { return "Renderer"; };

    virtual json serialize() const override;
    virtual void deserialize(const json& value) override;

    virtual void on_start() override {};
    virtual void on_update(const tmt::FrameData&) override {};
    virtual void on_end() override {};
};

}  // namespace tmt

TMT_OBJECT(tmt::RendererSettings, (bloom_radius));