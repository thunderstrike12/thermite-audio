#pragma once

#include <glm/glm.hpp>

#include <graphite/resources/handle.hh>

#include "engine/core/entity.hpp"

class RenderGraph;

namespace tmt {

/* Represents a renderable view in the scene. (used on the GPU) */
struct GpuView {
    /* World-space to clip-space transformation matrix. */
    glm::mat4 world_to_clip {};
    /* Clip-space to world-space transformation matrix. */
    glm::mat4 clip_to_world {};
    /* Origin of the view in world-space. */
    glm::vec4 origin {};
    /* Resolution of the view in pixels. */
    glm::uvec2 resolution {};
};

class RenderView {
   public:
    RenderView() = default;
    ~RenderView() = default;

    void init();
    void update();
    void update_gpu_view(RenderGraph& render_graph, Entity cam_entity);
    void deinit();

    /* If we are running the editor, we render to the viewport_image, otherwise we render directly to the render_target */
    BindHandle get_render_image() const;

    void set_viewport_size(uint32_t width, uint32_t height);

    /* Renderer output */
    Texture viewport_texture {};
    Image viewport_image {};
    u64 imgui_viewport {};

    RenderTarget render_target {};

    Buffer render_view_buffer {};

    GpuView gpu_view {};
};

}  // namespace tmt
