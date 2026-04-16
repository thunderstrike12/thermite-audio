#pragma once

#include <graphite/resources/handle.hh>

#include "core/renderer/render_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

class PostProcessPipeline {
   public:
    PostProcessPipeline() {}
    ~PostProcessPipeline() {}

    PostProcessPipeline(const PostProcessPipeline&) = delete;
    PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);

    /* Partial AutoX texture (256x256) */
    Texture autox_partial_texture {};
    Image autox_partial_image {};
    /* Tiny AutoX texture (16x16) */
    Texture autox_tiny_texture {};
    Image autox_tiny_image {};
    /* Final AutoX texture (1x1) */
    Texture autox_texture {};
    Image autox_image {};

    Sampler down_sampler {};
    Sampler up_sampler {};
};

}  // namespace tmt