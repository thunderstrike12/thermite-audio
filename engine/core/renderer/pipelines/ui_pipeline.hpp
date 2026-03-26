#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/renderer/render_view.hpp"
#include "engine/core/renderer/scene_view.hpp"
#include "engine/events/engine.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

constexpr uint32_t MAX_UI_IMAGES = 256u;
constexpr uint32_t MAX_UI_GLYPHS = 4096u;

class UiPipeline {
   private:
    /* Image Rendering */
    Buffer images_buffer {};
    Buffer image_vertex_buffer {};
    Sampler image_sampler {};

    /* Text Rendering */
    Buffer glyphs_buffer {};
    Buffer glyph_vertex_buffer {};
    Sampler text_sampler {};
    uint32_t glyph_count = 0u;

   public:
    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView& render_view);

    void enqueue_images(RenderGraph& render_graph, RenderView& render_view);
    void enqueue_text(RenderGraph& render_graph, RenderView& render_view);

    void deinit(GPUAdapter& gpu);

    UiPipeline() = default;
    ~UiPipeline() = default;

    /* Cannot be copied */
    UiPipeline(const UiPipeline&) = delete;
    UiPipeline& operator=(const UiPipeline&) = delete;

    uint32_t image_count = 0u;

    bool render_ui_pipeline = true;
    bool render_text = true;
};

}  // namespace tmt