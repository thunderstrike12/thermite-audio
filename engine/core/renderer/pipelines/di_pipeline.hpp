#pragma once

#include <graphite/resources/handle.hh>

class RenderGraph;
class GPUAdapter;

namespace tmt {

struct RenderView;
struct SceneView;

/* Direct Illumination Shading Rate. */
enum class ShadingRate : uint32_t {
    FULL_RATE = 0u,    /* Perform shading for every pixel on screen. */
    HALF_RATE = 1u,    /* Perform shading for half the pixels on screen. */
    QUARTER_RATE = 2u, /* Perform shading for 1/4th the pixels on screen. */
};

struct DiSettings {
    /* Performance setting to reduce shading cost. */
    ShadingRate shading_rate = ShadingRate::FULL_RATE;
};

constexpr uint32_t CACHE_SIZE = 10000000u;

/* Direct Illumination (DI) pipeline. */
class DiPipeline {
    /* Direct illumination settings buffer */
    DiSettings settings {};
    Buffer settings_buffer {};
    bool settings_dirty = true;

   public:
    DiPipeline() {}
    ~DiPipeline() {}

    DiPipeline(const DiPipeline&) = delete;
    DiPipeline& operator=(const DiPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view);
    void deinit(GPUAdapter& gpu);

    /* Get the shading rate of the DI pipeline. */
    inline ShadingRate get_shading_rate() const { return settings.shading_rate; };
    /* Set the shading rate of the DI pipeline. */
    inline void set_shading_rate(ShadingRate shading_rate) {
        settings.shading_rate = shading_rate;
        settings_dirty = true;
    };
};

}  // namespace tmt
