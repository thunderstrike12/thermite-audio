#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/renderer/render_view.hpp"
#include "engine/core/renderer/scene_view.hpp"
#include "engine/events/engine.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

constexpr uint32_t MAX_UI_IMAGES = 256;

struct ImageVertex {
    glm::vec3 pos {};
    glm::vec2 uvs {};
};

struct GpuImage {
    glm::vec2 pos {};
    glm::vec2 extent {};
    glm::vec4 color {};
    glm::vec3 angles {};
    uint32_t image_index {};
    glm::vec2 pivot {};
    float d0 = {}, d1 = {}; /* dummies */
};

class UiPipeline {
   private:
    Buffer images_buffer {};
    Buffer image_vertex_buffer {};
    Sampler image_sampler {};

    uint32_t image_count = 0;

   public:
    void init(GPUAdapter& gpu);
    // void on_engine_update(const FrameData& time) override;
    void enqueue(RenderGraph& render_graph, RenderView& render_view);
    void deinit(GPUAdapter& gpu);

    UiPipeline() = default;
    ~UiPipeline() = default;

    /* Cannot be copied */
    UiPipeline(const UiPipeline&) = delete;
    UiPipeline& operator=(const UiPipeline&) = delete;

    bool render_ui_pipeline = true;
};

}  // namespace tmt