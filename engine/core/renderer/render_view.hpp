#pragma once

#include <glm/glm.hpp>

#include <graphite/resources/handle.hh>

#include "engine/core/entity.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/shared/ray.hpp"

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
    glm::uvec2 resolution {1, 1};
    /* Index of the current frame. */
    glm::uint frame_index = 0u;
    /* Delta Time in seconds. */
    glm::float32 dt {};
};

/* Screen buffer resource. */
struct ScreenBuffer {
    Texture texture {};
    Image image {};
};

struct RenderView {
    RenderView() = default;
    ~RenderView() = default;

    void init();
    void update();
    void update_gpu_view(RenderGraph& render_graph, const Camera& camera, const Transform& transform);
    void deinit();

    /* Returns the viewport image if we're in the editor, or the render target if we're in the game. */
    BindHandle get_render_image() const;

    /* Update the size of the viewport. */
    void set_viewport_size(uint32_t width, uint32_t height);

    /* Create a ray from this render view for a given pixel coordinate. */
    Ray pixel_ray(glm::ivec2 pixel) const;

    /* Screen buffers */
    ScreenBuffer vbuffer {}; /* Visibility buffer (WxH, 6->8 bytes) */
    ScreenBuffer dbuffer {}; /* Depth buffer (WxH, 4 bytes) */
    ScreenBuffer ibuffer {}; /* Illuminance buffer (WxH, 4 bytes) */

    /* Macrofacet buffers */
    Buffer macrofacet_hashset {};           /* Macrofacet hash set buffer (WxH, 8 bytes) */
    Buffer macrofacet_shading_commands {};  /* List of (unique) shading commands (WxH, 8 bytes) */
    Buffer macrofacet_illuminance_cache {}; /* Macrofacet illuminance hash cache (10.000.000, 16 bytes) */

    /* Renderer output */
    ScreenBuffer viewport {}; /* Editor viewport */
    u64 imgui_viewport {};

    /* Render view constant buffer */
    Buffer render_view_buffer {};
    GpuView gpu_view {};

    /* (Final) Render target */
    RenderTarget render_target {};
    glm::uint frame_counter = 0u;

   private:
    void resize_textures();
};

}  // namespace tmt
