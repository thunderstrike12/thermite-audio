#pragma once

#include <glm/glm.hpp>

#include <graphite/resources/handle.hh>

#include "engine/core/entity.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/resources/texture_2d.hpp"
#include "engine/core/resources/envmap.hpp"
#include "engine/shared/ray.hpp"

class RenderGraph;

namespace tmt {

/* Represents a renderable view in the scene. (used on the GPU) */
struct GpuView {
    /* World-space to clip-space transformation matrix. */
    glm::mat4 world_to_clip = glm::mat4(1.f);
    /* Prev frame world-space to clip-space transformation matrix. */
    glm::mat4 prev_world_to_clip = glm::mat4(1.f);
    /* Clip-space to world-space transformation matrix. */
    glm::mat4 clip_to_world = glm::mat4(1.f);
    /* Origin of the view in world-space. */
    glm::vec4 origin {};
    /* Resolution of the view in pixels. */
    glm::uvec2 resolution { 1, 1 };
    /* Index of the current frame. */
    glm::uint frame_index = 0u;
    /* Delta Time in seconds. */
    glm::float32 dt {};
    /* Shading rate DI. (0 = 1/1, 1 = 1/2, 2 = 1/4) */
    glm::uint shading_rate_di = 0u;
    /* Shading rate GI. (0 = 1/1, 1 = 1/2, 2 = 1/4) */
    glm::uint shading_rate_gi = 2u;
    /* Current frame jitter offset */
    glm::vec2 jitter {};
    /* Prev frame jitter offset */
    glm::vec2 prev_jitter {};
};

/* Screen buffer resource. */
struct ScreenBuffer {
    Texture texture {};
    Image image {};
};

/* Direct Illumination Shading Rate. */
enum class ShadingRate : uint32_t {
    FULL_RATE = 0u,    /* Perform shading for every pixel on screen. */
    HALF_RATE = 1u,    /* Perform shading for half the pixels on screen. */
    QUARTER_RATE = 2u, /* Perform shading for 1/4th the pixels on screen. */
};

struct RenderView {
    RenderView() = default;
    ~RenderView() = default;

    void init();
    void update();
    void update_gpu_view(RenderGraph& render_graph, const Camera& camera, const Transform& transform);
    void deinit();

    /* Get the shading rate. */
    inline ShadingRate get_shading_rate_di() const { return shading_rate_di; };
    inline ShadingRate get_shading_rate_gi() const { return shading_rate_gi; };
    /* Set the shading rate. */
    inline void set_shading_rate_di(ShadingRate new_shading_rate) {
        if (shading_rate_di == new_shading_rate) return;
        shading_rate_di = new_shading_rate;
        resize_textures();
    };

    /* Returns the viewport image if we're in the editor, or the render target if we're in the game. */
    BindHandle get_render_image() const;

    /* Update the size of the viewport. */
    void set_viewport_size(uint32_t width, uint32_t height);

    /* Create a ray from this render view for a given pixel coordinate. */
    Ray pixel_ray(glm::ivec2 pixel) const;

    /* Directional albedo look up texture (32x32) */
    Texture diralbedo_lut_texture {};
    Image diralbedo_lut {};
    /* Blue noise textures (512x512) */
    ResourceRef<Texture2D> blue_noise2d {};
    ResourceRef<Texture2D> blue_noise1d {};

    /* Screen buffers */
    ScreenBuffer vbuffer {};  /* Visibility buffer (WxH, 6->8 bytes) */
    ScreenBuffer dbuffer {};  /* Depth buffer (WxH, 4 bytes) */
    ScreenBuffer lbuffer {};  /* Raw luminance buffer (WxH, 4 bytes) */
    ScreenBuffer nbuffer {};  /* Denoised luminance buffer (WxH, 4 bytes) */
    ScreenBuffer hbuffer1 {}; /* Accumulated (History) frame buffer (WxH, 8 bytes) */
    ScreenBuffer hbuffer2 {}; /* Accumulated (History) frame buffer (WxH, 8 bytes) */
    ScreenBuffer mbuffer {};  /* Motion Vector buffer (WxH, 4 bytes) */

    /* Macrofacet buffers */
    Buffer macrofacet_cache {}; /* Macrofacet hash cache (10.000.000, 48 bytes) */

    /* Renderer output */
    ScreenBuffer viewport {}; /* Editor viewport */
    u64 imgui_viewport {};

    /* Render view constant buffer */
    Buffer render_view_buffer {};
    GpuView gpu_view {};

    /* (Final) Render target */
    RenderTarget render_target {};
    glm::uint frame_counter = 0u;

    /* Prev Frame */
    glm::mat4 prev_world_to_clip = glm::mat4(1.f);
    glm::vec2 prev_jitter { 0.0f };

   private:
    ShadingRate shading_rate_di = ShadingRate::FULL_RATE;
    ShadingRate shading_rate_gi = ShadingRate::QUARTER_RATE;
    void resize_textures();
};

}  // namespace tmt
